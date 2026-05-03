#include <format>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <netinet/tcp.h> // TCP_NODELAY

#include "inet_address.h"
#include "socket.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "usage: ./tcpepoll ip port\n";
        std::cout << "example: ./tcpepoll 192.168.150.128 5085\n\n";
        return -1;
    }

    net::Socket server_sock(net::CreateNonBlocking());
    net::InetAddress server_addr{ argv[1], static_cast<uint16_t>(std::atoi(argv[2])) };
    server_sock.SetReuseAddr(true);
    server_sock.SetReusePort(true);
    server_sock.SetKeepalive(true);
    server_sock.SetTcpNodelay(true);
    server_sock.Bind(server_addr);
    server_sock.Listen();

    int epollfd = ::epoll_create(1); // 创建epoll句柄（红黑树）。

    // 为服务端的listenfd准备读事件。
    epoll_event ev;
    ev.data.fd = server_sock.fd(); // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回。
    ev.events = EPOLLIN; // 让epoll监视listenfd的读事件，采用水平触发。

    ::epoll_ctl(epollfd, EPOLL_CTL_ADD, server_sock.fd(), &ev); // 把需要监视的listenfd和它的事件加入epollfd中。

    epoll_event evs[10]; // 存放epoll_wait()返回事件的数组。

    while (true) // 事件循环。
    {
        int infds = ::epoll_wait(epollfd, evs, 10, -1); // 等待监视的fd有事件发生。

        // 返回失败。
        if (infds < 0)
        {
            ::perror("epoll_wait() failed");
            break;
        }

        // 超时。
        if (infds == 0)
        {
            std::cout << "epoll_wait() timeout.\n";
            continue;
        }

        // 如果infds>0，表示有事件发生的fd的数量。
        for (int i = 0; i < infds; i++) // 遍历epoll返回的数组evs。
        {
            if (evs[i].events & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
            {
                std::cout << std::format("client(eventfd={}) disconnected.\n", evs[i].data.fd);
                ::close(evs[i].data.fd); // 关闭客户端的fd。
            } //  普通数据  带外数据
            else if (evs[i].events & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
            {
                if (evs[i].data.fd == server_sock.fd()) // 如果是listenfd有事件，表示有新的客户端连上来。
                {
                    net::InetAddress client_addr{};
                    auto client_sock = net::Socket{ server_sock.Accept(client_addr) };

                    std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n", client_sock.fd(), client_addr.ip(), client_addr.port());

                    // 为新客户端连接准备读事件，并添加到epoll中。
                    ev.data.fd = client_sock.fd();
                    ev.events = EPOLLIN | EPOLLET; // 边缘触发。
                    ::epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock.fd(), &ev);
                }
                else // 如果是客户端连接的fd有事件。
                {
                    char buffer[1024];
                    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
                    {
                        ::bzero(&buffer, sizeof(buffer));
                        ssize_t nread = read(evs[i].data.fd, buffer, sizeof(buffer));
                        if (nread > 0) // 成功的读取到了数据。
                        {
                            // 把接收到的报文内容原封不动的发回去。
                            std::cout << std::format("recv(eventfd={}):{}\n", evs[i].data.fd, buffer);
                            ::send(evs[i].data.fd, buffer, strlen(buffer), 0);
                        }
                        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
                        {
                            continue;
                        }
                        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
                        {
                            break;
                        }
                        else if (nread == 0) // 客户端连接已断开。
                        {
                            std::cout << std::format("client(eventfd={}) disconnected.\n", evs[i].data.fd);
                            ::close(evs[i].data.fd); // 关闭客户端的fd。
                            break;
                        }
                    }
                }
            }
            else if (evs[i].events & EPOLLOUT) // 有数据需要写，暂时没有代码，以后再说。
            {
            }
            else // 其它事件，都视为错误。
            {
                std::cout << std::format("client(eventfd={}) error.\n", evs[i].data.fd);
                ::close(evs[i].data.fd); // 关闭客户端的fd。
            }
        }
    }

    return 0;
}