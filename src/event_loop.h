#pragma once

#include <memory>

namespace net
{
    class Epoll;
    class Channel;

    class EventLoop
    {
    public:
        EventLoop();
        ~EventLoop();

        // 运行事件循环
        void Run();
        
        Epoll* ep() const;
        void UpdateChannel(Channel *ch);
    private:
        std::unique_ptr<Epoll> ep_;
    };
};