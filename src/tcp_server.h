#pragma once

#include <cstdint>
#include <unordered_map>

#include "event_loop.h"

class Acceptor;
class Socket;
class Connection;

class TcpServer {
 public:
  TcpServer(const std::string& ip, uint16_t port);
  ~TcpServer();

  void Start();

  void HandleNewConnection(Socket* client_sock);

 private:
  EventLoop loop_;
  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<int, Connection*> conns_;
};