#include "tcp_server.h"

#include <format>
#include <iostream>

#include "acceptor.h"
#include "connection.h"


namespace net
{
    TcpServer::TcpServer(const std::string& ip, uint16_t port) : acceptor_{ std::make_unique<Acceptor>(&loop_, ip, port) }
    {
        acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::NewConnection, this, std::placeholders::_1));
    }
    TcpServer::~TcpServer() = default;

    void TcpServer::Start()
    {
        loop_.Run();
    }

    void TcpServer::NewConnection(std::unique_ptr<Socket> client_sock)
    {
        auto conn = std::make_unique<Connection>(&loop_, std::move(client_sock));
        std::cout << std::format("new connection(fd={},ip={},port={}) ok.\n", conn->fd(), conn->ip(), conn->port());
        conns_.emplace(conn->fd(), std::move(conn));
    }
}