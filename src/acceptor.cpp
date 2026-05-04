#include "acceptor.h"

#include "socket.h"
#include "channel.h"
#include "event_loop.h"

namespace net
{
    Acceptor::Acceptor(EventLoop* loop, const std::string& ip, uint16_t port) : loop_{ loop }
    {
        net::InetAddress server_addr{ ip, port };
        server_sock_->SetReuseAddr(true);
        server_sock_->SetReusePort(true);
        server_sock_->SetKeepalive(true);
        server_sock_->SetTcpNodelay(true);
        server_sock_->Bind(server_addr);
        server_sock_->Listen();

        auto server_channel = std::make_unique<Channel>(loop_, server_sock_->fd());
        server_channel->EnableReading();
        server_channel->SetReadCallback(std::bind(&Channel::NewConnection, server_channel.get(), *server_sock_));
    }

    Acceptor::~Acceptor() = default;
}