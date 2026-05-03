#include <format>
#include <iostream>
#include <memory>
#include <string_view>
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
#include "epoll.h"
#include "channel.h"

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

    net::Epoll ep;
    auto server_channel = std::make_unique<net::Channel>(&ep, server_sock.fd());
    server_channel->EnableReading();

    while (true) // 事件循环。
    {
        auto channels = ep.Wait();

        for (auto ch : channels)
        {
            const int evfd = ch->fd(); // 拷贝：epoll_event.data.fd 为 packed，不宜直接传入 std::format
            if (ch->events() & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
            {
                std::cout << std::format("client(eventfd={}) disconnected.\n", evfd);
                ::close(evfd); // 关闭客户端的fd。
            } //  普通数据  带外数据
            else if (ch->events() & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
            {
                if (ch == server_channel.get()) // 如果是listenfd有事件，表示有新的客户端连上来。
                {
                    net::InetAddress client_addr{};
                    auto client_sock = net::Socket{ server_sock.Accept(client_addr) };

                    std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n", client_sock.fd(), client_addr.ip(), client_addr.port());

                    // 为新客户端连接准备读事件，并添加到epoll中。
                    auto client_channel = std::make_unique<net::Channel>(&ep, client_sock.fd());
                    client_channel->UseET();
                    client_channel->EnableReading();
                }
                else // 如果是客户端连接的fd有事件。
                {
                    std::array<char, 1024> buffer;
                    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
                    {
                        buffer.fill('0');
                        ssize_t nread = ::read(evfd, buffer.data(), buffer.size());
                        if (nread > 0) // 成功的读取到了数据。
                        {
                            // 把接收到的报文内容原封不动的发回去。
                            std::cout << std::format("recv(eventfd={}):{}\n", evfd, std::string_view(buffer.data(), static_cast<size_t>(nread)));
                            ::send(evfd, buffer.data(), nread, 0);
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
                            std::cout << std::format("client(eventfd={}) disconnected.\n", evfd);
                            ::close(evfd); // 关闭客户端的fd。
                            break;
                        }
                    }
                }
            }
            else if (ch->events() & EPOLLOUT) // 有数据需要写，暂时没有代码，以后再说。
            {
            }
            else // 其它事件，都视为错误。
            {
                std::cout << std::format("client(eventfd={}) error.\n", evfd);
                ::close(evfd); // 关闭客户端的fd。
            }
        }
    }

    return 0;
}