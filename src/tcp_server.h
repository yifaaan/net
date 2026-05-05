#pragma once

#include <cstdint>

#include "event_loop.h"

class Acceptor;
class Socket;

class TcpServer {
 public:
  TcpServer(const std::string& ip, uint16_t port);
  ~TcpServer();

  void Start();

  void HandleNewConnection(Socket* client_sock);

 private:
  EventLoop loop_;
  std::unique_ptr<Acceptor> acceptor_;
};