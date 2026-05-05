#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>  // TCP_NODELAY
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <format>
#include <iostream>
#include <string_view>

#include "epoll.h"
#include "inet_address.h"
#include "socket.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << std::format("usage: ./tcpepoll ip port\n");
    std::cout << std::format("example: ./tcpepoll 192.168.150.128 5085\n\n");
    return -1;
  }

  InetAddress serv_addr(argv[1], atoi(argv[2]));
  // 创建服务端用于监听的listenfd。
  int listen_fd = CreateNonBlocking();
  Socket serv_sock{listen_fd};
  serv_sock.SetNodelay(true);
  serv_sock.SetReUseAddr(true);
  serv_sock.Bind(serv_addr);
  serv_sock.Listen();

  Epoll epoll;
  // 让epoll监视listen_fd的读事件，采用水平触发。
  epoll.AddFd(listen_fd, EPOLLIN);

  std::vector<epoll_event> revents;

  while (true) {
    revents = epoll.Loop();

    for (auto ev : revents)  // 遍历epoll返回的数组evs。
    {
      const int evfd = ev.data.fd;
      if (ev.events & EPOLLRDHUP) {
        // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
        std::cout << std::format("1client(eventfd={}) disconnected.\n", evfd);
        close(evfd);  // 关闭客户端的fd。
      } else if (ev.events & (EPOLLIN | EPOLLPRI)) {
        //  普通数据  带外数据
        // 接收缓冲区中有数据可以读。
        if (evfd == listen_fd) {
          sockaddr_in addr{};
          InetAddress client_addr;
          // 接收客户端连接
          auto client_sock = new Socket{serv_sock.Accept(client_addr)};
          int client_fd = client_sock->fd();

          std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n",
                                   client_fd, client_addr.ip(),
                                   client_addr.port());

          // 为新客户端连接准备读事件，并添加到epoll中。
          epoll.AddFd(client_fd, EPOLLIN | EPOLLET);  // 边缘触发。
        } else {
          char buffer[1024];
          // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
          while (true) {
            bzero(&buffer, sizeof(buffer));
            ssize_t nread = read(evfd, buffer, sizeof(buffer));
            if (nread > 0)  // 成功的读取到了数据。
            {
              // 把接收到的报文内容原封不动的发回去。
              std::cout << std::format(
                  "recv(eventfd={}):{}\n", evfd,
                  std::string_view(buffer, static_cast<size_t>(nread)));
              send(evfd, buffer, static_cast<size_t>(nread), 0);
            } else if (nread == -1 && errno == EINTR) {
              // 读取数据的时候被信号中断，继续读取。
              continue;
            } else if (nread == -1 &&
                       ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
              // 全部的数据已读取完毕。
              break;
            } else if (nread == 0) {  // 客户端连接已断开。
              std::cout << std::format("client(eventfd={}) disconnected.\n",
                                       evfd);
              close(evfd);  // 关闭客户端的fd。
              break;
            }
          }
        }
      } else if (ev.events & EPOLLOUT) {
        // 有数据需要写
      } else {  // 其它事件，都视为错误。
        std::cout << std::format("client(eventfd={}) error.\n", evfd);
        close(evfd);  // 关闭客户端的fd。
      }
    }
  }

  return 0;
}