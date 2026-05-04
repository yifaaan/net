#pragma once

#include <deque>
#include <atomic>
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
        // 线程安全：请求退出事件循环，Run() 将在本轮/下一轮唤醒后返回。
        void Stop();

        // epoll_wait 超时时间（毫秒）。默认 -1 表示一直阻塞，不会触发超时回调。
        void SetWaitTimeoutMs(int timeout_ms);
        void SetTimeoutCallback(std::function<void(EventLoop*)> cb);
        /** 线程安全：异步唤醒事件循环，打断阻塞中的 epoll_wait。 */
        void Wakeup();

        /** 线程安全：将任务投递到本 loop，在下一次 epoll 返回后在 loop 线程执行。 */
        void RunLater(std::function<void()> fn);

        Epoll* ep() const;
        void UpdateChannel(Channel *ch);
        void RemoveChannel(Channel* ch);
    private:
        void WakeupRead();
        void WakeupWrite();

        std::unique_ptr<Epoll> ep_;
        std::atomic<bool> quit_{ false };
        int wait_timeout_ms_{ -1 };
        std::function<void(EventLoop*)> timeout_callback_;

        int wakeup_fd_{ -1 };
        std::unique_ptr<Channel> wakeup_channel_;
        std::mutex pending_mutex_;
        std::deque<std::function<void()>> pending_functors_;
    };
};