#include "connection.h"

#include "event_loop.h"
#include "socket.h"
#include "channel.h"
#include <format>
#include <iostream>
#include <unistd.h>

namespace net
{
    Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> client_sock) : loop_{ loop }, client_sock_{ std::move(client_sock) }, channel_{ std::make_unique<Channel>(loop_, client_sock_->fd()) }
    {
        // 为新客户端连接准备读事件，并添加到epoll中。
        channel_->UseET();
        channel_->EnableReading();
        channel_->SetReadCallback(std::bind(&Channel::OnMessage, channel_.get()));
        channel_->SetCloseCallback(std::bind(&Connection::CloseCallback, this));
        channel_->SetErrorCallback(std::bind(&Connection::ErrorCallback, this));
    }
    Connection::~Connection() = default;

    int Connection::fd() const
    {
        return client_sock_->fd();
    }
    uint16_t Connection::port() const
    {
        return client_sock_->port();
    }
    const std::string& Connection::ip() const
    {
        return client_sock_->ip();
    }

    // 连接断开的回调
    void Connection::CloseCallback()
    {
        close_callback_(this);
    }

    // 错误的回调
    void Connection::ErrorCallback()
    {
        error_callback_(this);
    }

    void Connection::SetCloseCallback(std::function<void(Connection*)> cb)
    {
        close_callback_ = std::move(cb);
    }
    void Connection::SetErrorCallback(std::function<void(Connection*)> cb)
    {
        error_callback_ = std::move(cb);
    }
}