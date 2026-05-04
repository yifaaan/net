#include "connection.h"

#include "event_loop.h"
#include "socket.h"
#include "channel.h"
#include <format>
#include <iostream>

namespace net
{
    Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> client_sock) : loop_{ loop }, client_sock_{ std::move(client_sock) }, channel_{ std::make_unique<Channel>(loop_, client_sock_->fd()) }
    {
        // 为新客户端连接准备读事件，并添加到epoll中。
        channel_->UseET();
        channel_->EnableReading();
        channel_->SetReadCallback(std::bind(&Channel::OnMessage, channel_.get()));
    }
    Connection::~Connection() = default;
}