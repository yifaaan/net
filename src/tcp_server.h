#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "event_loop.h"
#include "socket.h"


namespace net
{
    class Acceptor;
    class Connection;

    class TcpServer
    {
    public:
        TcpServer(const std::string& ip, uint16_t port);
        ~TcpServer();

        // 运行事件循环
        void Start();

        void NewConnection(std::unique_ptr<Socket> client_sock);

        void CloseConnection(Connection* conn); // 在Connection中回调
        void ErrorConnection(Connection* conn); // 在Connection中回调

    private:
        EventLoop loop_;
        std::unique_ptr<Acceptor>  acceptor_;

        std::unordered_map<int, std::unique_ptr<Connection>> conns_;
    };
}