#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

class InetAddress {
 public:
  // server 构造监听的 socket 地址
  InetAddress(const std::string& ip, uint16_t port);
  // server 构造client连接的 socket 地址
  InetAddress(sockaddr_in addr);

  const char* ip() const;
  uint16_t port() const;
  // 获取 socket 地址 bind使用
  const sockaddr* addr() const;

 private:
  sockaddr_in addr_;
};