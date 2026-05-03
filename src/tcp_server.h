#pragma once

#include <cstdint>

#include "event_loop.h"
#include "socket.h"


namespace net
{
    class TcpServer
    {
    public:
        TcpServer(const std::string& ip, uint16_t port);
        ~TcpServer();

        // 运行事件循环
        void Start();
    private:
        EventLoop loop_;
        Socket server_sock_;
    };
}