#include "tcp_server.h"

#include "acceptor.h"
#include "channel.h"
#include "connection.h"
#include "socket.h"

TcpServer::TcpServer(const std::string& ip, uint16_t port)
    : acceptor_{std::make_unique<Acceptor>(&loop_, ip, port)} {
  // Acceptor 收到客户连接时的回调
  acceptor_->SetNewConnectionCallback(
      [this](Socket* client_sock) { HandleNewConnection(client_sock); });
}

TcpServer::~TcpServer() = default;

void TcpServer::Start() { loop_.Run(); }

void TcpServer::HandleNewConnection(Socket* client_sock) {
  Connection* conn = new Connection{&loop_, client_sock};
}