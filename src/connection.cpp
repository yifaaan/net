#include "connection.h"

#include "event_loop.h"
#include "socket.h"
#include "channel.h"
#include <format>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace net
{
    Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> client_sock) : loop_{ loop }, client_sock_{ std::move(client_sock) }, channel_{ std::make_unique<Channel>(loop_, client_sock_->fd()) }
    {
        // 为新客户端连接准备读事件，并添加到epoll中。
        channel_->UseET();
        channel_->EnableReading();
        channel_->SetReadCallback(std::bind(&Connection::OnMessage, this));
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

    // 处理对端发来的消息
    void Connection::OnMessage()
    {
        std::array<char, 1024> buffer;
        while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
        {
            buffer.fill('0');
            ssize_t nread = ::read(fd(), buffer.data(), buffer.size());
            if (nread > 0) // 成功的读取到了数据。
            {
                // 把接收到的报文内容原封不动的发回去。
                std::cout << std::format("recv(eventfd={}):{}\n", fd(), std::string_view(buffer.data(), static_cast<size_t>(nread)));
                input_buffer_.Append(buffer.data(), nread);
            }
            else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
            {
                continue;
            }
            else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
            {
                output_buffer_ = input_buffer_;
                ::send(fd(), output_buffer_.data(), output_buffer_.size(), 0);
                input_buffer_.Clear();
                break;
            }
            else if (nread == 0) // 客户端连接已断开。
            {
                close_callback_(this);
                break;
            }
        }
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