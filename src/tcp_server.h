#pragma once

#include <cstdint>

#include "event_loop.h"
#include "socket.h"


namespace net
{
    class Acceptor;

    class TcpServer
    {
    public:
        TcpServer(const std::string& ip, uint16_t port);
        ~TcpServer();

        // 运行事件循环
        void Start();

        void NewConnection(std::unique_ptr<Socket> client_sock);
    private:
        EventLoop loop_;
        std::unique_ptr<Acceptor>  acceptor_;
    };
}