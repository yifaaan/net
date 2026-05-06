#include <csignal>
#include <cstdlib>
#include <thread>

#include "echo_server.h"
#include "log.h"

int main(int argc, char* argv[]) {
  net::InitLog();

  if (argc != 3) {
    spdlog::error("component=net_main event=invalid_args usage=\"./tcpepoll ip port\"");
    spdlog::error("component=net_main event=usage_example example=\"./tcpepoll 192.168.150.128 5085\"");
    return -1;
  }

  // 用 sigwait 来触发优雅关闭（避免在 signal handler 里做非异步安全操作）
  // 注意：必须在创建任何线程之前就 block 信号（EchoServer/TcpServer 构造会启动线程池）
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, nullptr);

  EchoServer server{argv[1], static_cast<uint16_t>(std::atoi(argv[2])), 5};

  std::jthread signal_thread([&](std::stop_token) {
    int sig = 0;
    if (sigwait(&set, &sig) == 0) {
      spdlog::info("component=net_main event=signal_received sig={}", sig);
      server.Stop();
    }
  });

  server.Start();
}
