#pragma once

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>

#include "inet_address.h"

class Socket {
 public:
  explicit Socket(int fd) : fd_(fd) {}
  ~Socket() { ::close(fd_); }

  int fd() const { return fd_; }
  uint16_t port() const { return port_; }
  const std::string& ip() const { return ip_; }

  void SetIpPort(const std::string& ip, uint16_t port) {
    ip_ = ip;
    port_ = port;
  }

  void SetReUseAddr(bool on) {
    int op = on;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<void*>(&op),
                 sizeof(op));
  }

  void SetReUsePort(bool on) {
    int op = on;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<void*>(&op),
                 sizeof(op));
  }

  void SetKeepAlive(bool on) {
    int op = on;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<void*>(&op),
                 sizeof(op));
  }

  void SetNodelay(bool on) {
    int op = on;
    ::setsockopt(fd_, SOL_SOCKET, TCP_NODELAY, reinterpret_cast<void*>(&op),
                 sizeof(op));
  }

  void Bind(const InetAddress& addr) {
    if (::bind(fd_, addr.addr(), sizeof(sockaddr)) < 0) {
      spdlog::error("bind() failed: {}", std::strerror(errno));
      std::exit(-1);
    }
    SetIpPort(addr.ip(), addr.port());
  }

  void Listen(int n = 128) {
    if (::listen(fd_, n) < 0) {
      spdlog::error("listen failed: {}", std::strerror(errno));
      std::exit(-1);
    }
  }

  int Accept(InetAddress& client_addr) {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int client_fd =
        ::accept4(fd_, reinterpret_cast<sockaddr*>(&addr), &len, SOCK_NONBLOCK);
    client_addr.SetAddr(addr);

    return client_fd;
  }

 private:
  const int fd_{-1};
  std::string ip_;
  uint16_t port_{};
};

inline int CreateNonBlocking() {
  int listen_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
  if (listen_fd < 0) {
    spdlog::error("socket() failed: {}", std::strerror(errno));
    std::exit(-1);
  }
  return listen_fd;
}
