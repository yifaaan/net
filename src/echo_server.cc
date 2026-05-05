#include "echo_server.h"

#include <iostream>

#include "tcp_server.h"
#include "connection.h"

EchoServer::EchoServer(const std::string& ip, uint16_t port)
    : tcp_server_{ip, port} {
  tcp_server_.SetNewConnectionCallback(
      [this](Connection* conn) { HandleNewConnection(conn); });
  tcp_server_.SetCloseConnectionCallback(
      [this](Connection* conn) { HandleClose(conn); });
  tcp_server_.SetErrorConnectionCallback(
      [this](Connection* conn) { HandleError(conn); });
  tcp_server_.SetOnMessageCallback([this](Connection* conn, std::string& msg) {
    HandleMessage(conn, msg);
  });
  tcp_server_.SetSendCompleteCallback(
      [this](Connection* conn) { HandleSendComplete(conn); });
  tcp_server_.SetEpollTimeoutCallback(
      [this](EventLoop* loop) { HandleEpollTimeout(loop); });
}

EchoServer::~EchoServer() = default;

void EchoServer::Start() { tcp_server_.Start(); }

// 客户端连接时，TcpServer会回调该函数
void EchoServer::HandleNewConnection(Connection* conn) {
  std::cout << "new connection come in\n";
  (void)conn;
}

// 关闭客户端的连接，在Connection中回调
void EchoServer::HandleClose(Connection* conn) {
  std::cout << "connection closed\n";
}

// 处理客户端连接错误，在Connection中回调
void EchoServer::HandleError(Connection* conn) {
  std::cout << "connection error\n";
}

// 处理客户端的一条完整 请求报文，在Connection类中回调
void EchoServer::HandleMessage(Connection* conn, std::string& message) {
  message = "reply:" + message;

  // 回应报文
  // 将数据写入Connection的output_buffer
  conn->Send(message.data(), message.size());
}

// 数据发送完成后，在Connection类中回调
void EchoServer::HandleSendComplete(Connection* conn) {
  std::cout << "message send complete\n";
}

// epoll_wait超时后的回调，在EventLoop类中回调
void EchoServer::HandleEpollTimeout(EventLoop* loop) {
  std::cout << "epoll timeout\n";
}