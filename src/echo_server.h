#pragma once

#include <cstdint>
#include <string>

#include "tcp_server.h"

namespace net
{
    class Connection;
    class EventLoop;

    class EchoServer
    {
    public:
        EchoServer(const std::string& ip, uint16_t port);

        void Start();
        void SetEpollWaitTimeoutMs(int timeout_ms);

    private:
        void OnMessage(Connection* conn, std::string header, std::string payload);
        void OnSendComplete(Connection* conn);
        void OnTimeout(EventLoop* loop);

        TcpServer server_;
    };
}
