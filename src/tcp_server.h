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

  // 客户端连接时，Acceptor会回调该函数
  void HandleNewConnection(Socket* client_sock);


  // 关闭客户端的连接，在Connection中回调
  void HandleCloseConnection(Connection* conn);

  // 处理客户端连接错误，在Connection中回调
  void HandleErrorConnection(Connection* conn);

 private:
  EventLoop loop_;
  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<int, Connection*> conns_;
};