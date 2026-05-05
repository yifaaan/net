#include "tcp_server.h"

#include "acceptor.h"
#include "channel.h"
#include "socket.h"

TcpServer::TcpServer(const std::string& ip, uint16_t port)
    : acceptor_{std::make_unique<Acceptor>(&loop_, ip, port)} {}

TcpServer::~TcpServer() = default;

void TcpServer::Start() { loop_.Run(); }