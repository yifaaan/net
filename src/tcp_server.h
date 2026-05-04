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

        // epoll_wait 超时（毫秒）。须 >= 0 才会在每次超时时调用 OnTimeout；-1 表示永久阻塞（默认）。
        void SetEpollWaitTimeoutMs(int timeout_ms);

        void SetMessageHandler(std::function<void(Connection*, std::string, std::string)> cb);
        void SetSendCompleteHandler(std::function<void(Connection*)> cb);
        void SetTimeoutHandler(std::function<void(EventLoop*)> cb);

        void NewConnection(std::unique_ptr<Socket> client_sock);

        void CloseConnection(Connection* conn); // 在Connection中回调
        void ErrorConnection(Connection* conn); // 在Connection中回调

        void OnMessage(Connection* conn, std::string header, std::string payload);
        void OnSendComplete(Connection* conn);
        void OnTimeout(EventLoop* loop);
    private:
        EventLoop loop_;
        std::unique_ptr<Acceptor>  acceptor_;

        std::unordered_map<int, std::unique_ptr<Connection>> conns_;

        std::function<void(Connection*, std::string, std::string)> message_handler_;
        std::function<void(Connection*)> send_complete_handler_;
        std::function<void(EventLoop*)> timeout_handler_;
    };
}