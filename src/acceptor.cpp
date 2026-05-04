#include "acceptor.h"

#include <format>
#include <iostream>

#include "connection.h"
#include "socket.h"
#include "channel.h"
#include "event_loop.h"

namespace net
{
    Acceptor::Acceptor(EventLoop* loop, const std::string& ip, uint16_t port) : loop_{ loop }, server_sock_{ std::make_unique<Socket>(CreateNonBlocking()) }
    {
        net::InetAddress server_addr{ ip, port };
        server_sock_->SetReuseAddr(true);
        server_sock_->SetReusePort(true);
        server_sock_->SetKeepalive(true);
        server_sock_->SetTcpNodelay(true);
        server_sock_->Bind(server_addr);
        server_sock_->Listen();

        channel_ = std::make_unique<Channel>(loop_, server_sock_->fd());
        channel_->EnableReading();
        channel_->SetReadCallback(std::bind(&Acceptor::NewConnection, this));
    }

    Acceptor::~Acceptor() = default;

    void Acceptor::NewConnection()
    {
        net::InetAddress client_addr{};
        auto client_sock = std::make_unique<Socket>(server_sock_->Accept(client_addr));
        client_sock->SetIpPort(client_addr.ip(), client_addr.port());
        new_connection_callback_(std::move(client_sock));
    }

    void Acceptor::SetNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> cb)
    {
        new_connection_callback_ = std::move(cb);
    }
}