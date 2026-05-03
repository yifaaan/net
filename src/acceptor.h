#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace net
{
    class EventLoop;
    class Socket;
    class Channel;

    class Acceptor
    {
    public:
        Acceptor(EventLoop* loop, const std::string& ip, uint16_t port);
        ~Acceptor();

    private:
        // 一个Acceptor对应一个loop
        EventLoop* loop_;
        // 监听socket
        std::unique_ptr<Socket> server_sock_;
        std::unique_ptr<Channel> channel_;
    };
}