#pragma once

#include <cstdint>
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

        // epoll_wait 超时（毫秒）。须 >= 0 才会在每次超时时调用 OnTimeout；-1 表示永久阻塞（默认）。
        void SetEpollWaitTimeoutMs(int timeout_ms);

        void NewConnection(std::unique_ptr<Socket> client_sock);

        void CloseConnection(Connection* conn); // 在Connection中回调
        void ErrorConnection(Connection* conn); // 在Connection中回调

        void OnMessage(Connection* conn, std::string message);
        void OnSendComplete(Connection* conn);
        void OnTimeout();
    private:
        EventLoop loop_;
        std::unique_ptr<Acceptor>  acceptor_;

        std::unordered_map<int, std::unique_ptr<Connection>> conns_;
    };
}