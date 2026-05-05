#include <format>
#include <iostream>

#include "tcp_server.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << std::format("usage: ./tcpepoll ip port\n");
    std::cout << std::format("example: ./tcpepoll 192.168.150.128 5085\n\n");
    return -1;
  }

  TcpServer server{argv[1], static_cast<uint16_t>(std::atoi(argv[2]))};
  server.Start();
}