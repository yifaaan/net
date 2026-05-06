
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"

int main(int argc, char* argv[]) {
  net::InitLog();

  if (argc != 3) {
    spdlog::error("component=net_client event=invalid_args usage=\"./client ip port\"");
    spdlog::error("component=net_client event=usage_example example=\"./client 192.168.150.128 5085\"");
    return -1;
  }

  int sockfd;
  struct sockaddr_in servaddr;
  char buf[1024];

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    spdlog::error("component=net_client event=socket_failed error={}",
                  std::strerror(errno));
    return -1;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(atoi(argv[2]));
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);

  if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
    spdlog::error("component=net_client event=connect_failed ip={} port={} error={}",
                  argv[1], argv[2],
                  std::strerror(errno));
    close(sockfd);
    return -1;
  }

  spdlog::info("component=net_client event=connect_ok ip={} port={}", argv[1],
               argv[2]);
  // spdlog::info("component=net_client event=start timestamp={}", time(0));

  for (int ii = 0; ii < 1; ii++) {
    // 从命令行输入内容。
    ::memset(buf, 0, sizeof(buf));
    ::sprintf(buf, "这是第%d句话\n", ii);

    char tmp[1024]{};
    int len = strlen(buf);
    ::memcpy(tmp, &len, sizeof(len));
    ::memcpy(tmp + sizeof(len), buf, len);
    ::send(sockfd, tmp, len + sizeof(len), 0);
    spdlog::info("component=net_client event=send_request index={} bytes={} message={}",
                 ii, len, buf);
  }
  for (int ii = 0; ii < 1; ii++) {
    int len;
    if (::recv(sockfd, &len, sizeof(len), MSG_WAITALL) != sizeof(len)) {
      spdlog::error("component=net_client event=recv_header_failed error={}",
                    std::strerror(errno));
      break;
    }
    if (len <= 0 || len >= static_cast<int>(sizeof(buf))) {
      spdlog::error("component=net_client event=invalid_body_len bytes={}", len);
      break;
    }
    ::memset(buf, 0, sizeof(buf));
    if (::recv(sockfd, buf, len, MSG_WAITALL) != len) {
      spdlog::error("component=net_client event=recv_body_failed bytes={} error={}",
                    len, std::strerror(errno));
      break;
    }
    spdlog::info("component=net_client event=recv_reply index={} bytes={} message={}",
                 ii, len, buf);
  }

  sleep(100);
  // spdlog::info("component=net_client event=stop timestamp={}", time(0));
}
