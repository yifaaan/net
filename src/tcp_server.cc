#include "tcp_server.h"

#include <format>

#include "acceptor.h"
#include "channel.h"
#include "connection.h"
#include "socket.h"

TcpServer::TcpServer(const std::string& ip, uint16_t port)
    : acceptor_{std::make_unique<Acceptor>(&loop_, ip, port)} {
  // Acceptor 收到客户连接时，回调TcpServer的函数
  acceptor_->SetNewConnectionCallback(
      [this](Socket* client_sock) { HandleNewConnection(client_sock); });

  // 设置epoll_wait超时回调
  loop_.SetEpollTimeoutCallback(
      [this](EventLoop* loop) { EpollTimeout(loop); });
}

TcpServer::~TcpServer() {
  for (auto [_, conn] : conns_) {
    delete conn;
  }
}

void TcpServer::Start() { loop_.Run(); }

void TcpServer::HandleNewConnection(Socket* client_sock) {
  Connection* conn = new Connection{&loop_, client_sock};
  conns_[conn->fd()] = conn;
  // 设置事件发生时的回调
  conn->SetCloseCallback(
      [this](Connection* conn) { HandleCloseConnection(conn); });
  conn->SetErrorCallback(
      [this](Connection* conn) { HandleErrorConnection(conn); });
  conn->SetOnMessageCallback([this](Connection* conn, std::string& message) {
    OnMessage(conn, message);
  });
  conn->SetSendCompeleteCallbace(
      [this](Connection* conn) { SendComplete(conn); });
  std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n",
                           conn->fd(), conn->ip(), conn->port());

  // for echo server
  new_connection_callback_(conn);
}

void TcpServer::HandleCloseConnection(Connection* conn) {
  // for echo server
  close_connection_callback_(conn);

  // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
  // std::cout << std::format("1client(eventfd={}) disconnected.\n",
  // conn->fd());
  conns_.erase(conn->fd());
  delete conn;
}

void TcpServer::HandleErrorConnection(Connection* conn) {
  // for echo server
  error_connection_callback_(conn);

  // std::cout << std::format("client(eventfd={}) error.\n", conn->fd());
  conns_.erase(conn->fd());
  delete conn;
}

void TcpServer::OnMessage(Connection* conn, std::string& message) {
  // for echo server
  on_message_callback_(conn, message);

  // message = "reply:" + message;

  // // 回应报文
  // int len = message.size();
  // std::string tmp{reinterpret_cast<char*>(&len), sizeof(len)};
  // tmp += message;
  // // 将数据写入Connection的output_buffer
  // conn->Send(tmp.data(), tmp.size());
}

void TcpServer::SendComplete(Connection* conn) {
  // for echo server
  send_complete_callback_(conn);

  // ...
}

void TcpServer::EpollTimeout(EventLoop* loop) {
  // for echo server
  epoll_timeout_callback_(loop);

  // ...
}