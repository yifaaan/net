#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>

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
        void SetTimeoutCallback(std::function<void(EventLoop*)> cb);

        /** 线程安全：将任务投递到本 loop，在下一次 epoll 返回后在 loop 线程执行。 */
        void RunLater(std::function<void()> fn);

        Epoll* ep() const;
        void UpdateChannel(Channel *ch);
        void RemoveChannel(Channel* ch);
    private:
        void WakeupRead();

        std::unique_ptr<Epoll> ep_;
        int wait_timeout_ms_{ -1 };
        std::function<void(EventLoop*)> timeout_callback_;

        int wakeup_fd_{ -1 };
        std::unique_ptr<Channel> wakeup_channel_;
        std::mutex pending_mutex_;
        std::deque<std::function<void()>> pending_functors_;
    };
};