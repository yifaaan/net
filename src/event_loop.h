#pragma once

#include <functional>
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

        // epoll_wait 超时时间（毫秒）。默认 -1 表示一直阻塞，不会触发超时回调。
        void SetWaitTimeoutMs(int timeout_ms);
        void SetTimeoutCallback(std::function<void()> cb);

        Epoll* ep() const;
        void UpdateChannel(Channel *ch);
    private:
        std::unique_ptr<Epoll> ep_;
        int wait_timeout_ms_{ -1 };
        std::function<void()> timeout_callback_;
    };
};