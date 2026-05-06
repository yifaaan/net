#include <cstdlib>

#include "echo_server.h"
#include "log.h"

int main(int argc, char* argv[]) {
  net::InitLog();

  if (argc != 3) {
    spdlog::error("usage: ./tcpepoll ip port");
    spdlog::error("example: ./tcpepoll 192.168.150.128 5085");
    return -1;
  }

  EchoServer server{argv[1], static_cast<uint16_t>(std::atoi(argv[2])), 5};
  server.Start();
}
