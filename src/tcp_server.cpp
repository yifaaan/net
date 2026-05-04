#include "tcp_server.h"

#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <unistd.h>

#include "acceptor.h"
#include "connection.h"

namespace net
{
    TcpServer::TcpServer(const std::string& ip, uint16_t port) : acceptor_{ std::make_unique<Acceptor>(&loop_, ip, port) }
    {
        acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::NewConnection, this, std::placeholders::_1));
        loop_.SetTimeoutCallback([this](EventLoop* loop) { OnTimeout(loop); });
    }

    void TcpServer::SetEpollWaitTimeoutMs(int timeout_ms)
    {
        loop_.SetWaitTimeoutMs(timeout_ms);
    }

    void TcpServer::SetMessageHandler(std::function<void(Connection*, std::string)> cb)
    {
        message_handler_ = std::move(cb);
    }

    void TcpServer::SetSendCompleteHandler(std::function<void(Connection*)> cb)
    {
        send_complete_handler_ = std::move(cb);
    }

    void TcpServer::SetTimeoutHandler(std::function<void(EventLoop*)> cb)
    {
        timeout_handler_ = std::move(cb);
    }

    TcpServer::~TcpServer() = default;

    void TcpServer::Start()
    {
        loop_.Run();
    }

    void TcpServer::NewConnection(std::unique_ptr<Socket> client_sock)
    {
        auto conn = std::make_unique<Connection>(&loop_, std::move(client_sock));
        conn->SetCloseCallback(std::bind(&TcpServer::CloseConnection, this, std::placeholders::_1));
        conn->SetErrorCallback(std::bind(&TcpServer::ErrorConnection, this, std::placeholders::_1));
        conn->SetMessageCallback(std::bind(&TcpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
        conn->SetWriteCompleteCallback(std::bind(&TcpServer::OnSendComplete, this, std::placeholders::_1));
        std::cout << std::format("new connection(fd={},ip={},port={}) ok.\n", conn->fd(), conn->ip(), conn->port());

        conns_.emplace(conn->fd(), std::move(conn));
    }

    void TcpServer::CloseConnection(Connection* conn)
    {
        std::cout << std::format("client(eventfd={}) disconnected.\n", conn->fd());
        conns_.erase(conn->fd());
    }
    void TcpServer::ErrorConnection(Connection* conn)
    {
        std::cout << std::format("client(eventfd={}) error.\n", conn->fd());
        conns_.erase(conn->fd());
    }

    void TcpServer::OnMessage(Connection* conn, std::string message)
    {
        if (message_handler_)
            message_handler_(conn, std::move(message));
    }

    void TcpServer::OnSendComplete(Connection* conn)
    {
        if (send_complete_handler_)
            send_complete_handler_(conn);
    }

    void TcpServer::OnTimeout(EventLoop* loop)
    {
        if (timeout_handler_)
            timeout_handler_(loop);
    }
}