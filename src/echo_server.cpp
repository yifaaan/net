#include "echo_server.h"

#include <format>
#include <iostream>

#include "connection.h"

namespace net
{
    EchoServer::EchoServer(const std::string& ip, uint16_t port) : server_(ip, port)
    {
        server_.SetMessageHandler([this](Connection* c, std::string h, std::string p) { OnMessage(c, std::move(h), std::move(p)); });
        server_.SetSendCompleteHandler([this](Connection* c) { OnSendComplete(c); });
        server_.SetTimeoutHandler([this](EventLoop* l) { OnTimeout(l); });
    }

    void EchoServer::Start()
    {
        server_.Start();
    }

    void EchoServer::SetEpollWaitTimeoutMs(int timeout_ms)
    {
        server_.SetEpollWaitTimeoutMs(timeout_ms);
    }

    void EchoServer::OnMessage(Connection* conn, std::string header, std::string payload)
    {
        std::cout << std::format(
            "recv(eventfd={},ip={},port={}): header={} bytes payload={} bytes [{}]\n",
            conn->fd(),
            conn->ip(),
            conn->port(),
            header.size(),
            payload.size(),
            payload);
        conn->Send(payload.data(), payload.size());
    }

    void EchoServer::OnSendComplete(Connection* conn)
    {
        std::cout << std::format("send complete(eventfd={},ip={},port={})\n", conn->fd(), conn->ip(), conn->port());
    }

    void EchoServer::OnTimeout(EventLoop* loop)
    {
        std::cout << std::format("epoll_wait timeout (loop={})\n", static_cast<void*>(loop));
    }
}
