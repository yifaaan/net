
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("usage:./client ip port\n");
    printf("example:./client 192.168.150.128 5085\n\n");
    return -1;
  }

  int sockfd;
  struct sockaddr_in servaddr;
  char buf[1024];

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket() failed.\n");
    return -1;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(atoi(argv[2]));
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);

  if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
    printf("connect(%s:%s) failed.\n", argv[1], argv[2]);
    close(sockfd);
    return -1;
  }

  printf("connect ok.\n");
  // printf("开始时间：%d",time(0));

  for (int ii = 0; ii < 1; ii++) {
    // 从命令行输入内容。
    ::memset(buf, 0, sizeof(buf));
    ::sprintf(buf, "这是第%d句话\n", ii);

    char tmp[1024]{};
    int len = strlen(buf);
    ::memcpy(tmp, &len, sizeof(len));
    ::memcpy(tmp + sizeof(len), buf, len);
    ::send(sockfd, tmp, len + sizeof(len), 0);
  }
  for (int ii = 0; ii < 1; ii++) {
    int len;
    if (::recv(sockfd, &len, sizeof(len), MSG_WAITALL) != sizeof(len)) {
      printf("recv len failed.\n");
      break;
    }
    if (len <= 0 || len >= static_cast<int>(sizeof(buf))) {
      printf("invalid body len=%d\n", len);
      break;
    }
    ::memset(buf, 0, sizeof(buf));
    if (::recv(sockfd, buf, len, MSG_WAITALL) != len) {
      printf("recv body failed.\n");
      break;
    }
    printf("%s", buf);
  }

  sleep(100);
  // printf("结束时间：%d",time(0));
}