#include <cstdlib>

#include "echo_server.h"
#include "log.h"

int main(int argc, char* argv[]) {
  net::InitLog();

  if (argc != 3) {
    spdlog::error("component=net_main event=invalid_args usage=\"./tcpepoll ip port\"");
    spdlog::error("component=net_main event=usage_example example=\"./tcpepoll 192.168.150.128 5085\"");
    return -1;
  }

  EchoServer server{argv[1], static_cast<uint16_t>(std::atoi(argv[2])), 5};
  server.Start();
}
