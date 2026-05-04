#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace net
{
    class EventLoop;
    class Socket;
    class Channel;

    // 封装监听的channel
    class Acceptor
    {
    public:
        Acceptor(EventLoop* loop, const std::string& ip, uint16_t port);
        ~Acceptor();

        // 处理新连接
        void NewConnection();

        void SetNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> cb);
    private:
        // 一个Acceptor对应一个loop
        EventLoop* loop_;
        // 监听socket
        std::unique_ptr<Socket> server_sock_;
        std::unique_ptr<Channel> channel_;

        std::function<void(std::unique_ptr<Socket>)> new_connection_callback_;
    };
}