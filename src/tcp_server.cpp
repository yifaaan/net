#include "tcp_server.h"

#include "channel.h"
#include "socket.h"

namespace net
{
    TcpServer::TcpServer(const std::string& ip, uint16_t port) : server_sock_{ CreateNonBlocking() }
    {
        net::InetAddress server_addr{ ip, port };
        server_sock_.SetReuseAddr(true);
        server_sock_.SetReusePort(true);
        server_sock_.SetKeepalive(true);
        server_sock_.SetTcpNodelay(true);
        server_sock_.Bind(server_addr);
        server_sock_.Listen();

        auto server_channel = std::make_unique<Channel>(loop_.ep(), server_sock_.fd());
        server_channel->EnableReading();
        server_channel->SetReadCallback(std::bind(&Channel::NewConnection, server_channel.get(), server_sock_));
    }
    TcpServer::~TcpServer() = default;

    void TcpServer::Start()
    {
        loop_.Run();
    }
}