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

#include "channel.h"
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
  auto serv_sock = new Socket{listen_fd};
  serv_sock->SetNodelay(true);
  serv_sock->SetReUseAddr(true);
  serv_sock->Bind(serv_addr);
  serv_sock->Listen();

  Epoll epoll;
  // 让epoll监视listen_fd的读事件，采用水平触发。
  auto serv_channel = new Channel{&epoll, listen_fd};
  // server设置读回调，事件发生后 ，处理客户端连接
  serv_channel->SetReadCallback(
      [=] { serv_channel->HandleNewConnection(serv_sock); });

  serv_channel->EnableReading();

  std::vector<Channel*> channels;

  while (true) {
    // epoll_wait
    channels = epoll.Loop();

    for (auto ch : channels) {
      ch->HandleEvent();
    }
  }
}