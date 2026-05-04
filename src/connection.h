#pragma once

#include <memory>

namespace net
{

    class EventLoop;
    class Socket;
    class Channel;

    // 封装处理用户请求的channel
    class Connection
    {
    public:
        Connection(EventLoop* loop, std::unique_ptr<Socket> client_sock);
        ~Connection();

    private:
        // 一个Acceptor对应一个loop
        EventLoop* loop_;
        // 监听socket
        std::unique_ptr<Socket> client_sock_;
        std::unique_ptr<Channel> channel_;
    };
}