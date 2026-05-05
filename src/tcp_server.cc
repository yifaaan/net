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

  std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n",
                           conn->fd(), conn->ip(), conn->port());
}

void TcpServer::HandleCloseConnection(Connection* conn) {
  // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
  std::cout << std::format("1client(eventfd={}) disconnected.\n", conn->fd());
  conns_.erase(conn->fd());
  delete conn;
}

void TcpServer::HandleErrorConnection(Connection* conn) {
  std::cout << std::format("client(eventfd={}) error.\n", conn->fd());
  conns_.erase(conn->fd());
  delete conn;
}