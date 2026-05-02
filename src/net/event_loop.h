#pragma once

#include <chrono>
#include <atomic>
#include <functional>
#include <mutex>

#include "../base/noncopyable.h"
#include "../base/current_thread.h"

namespace net
{
    class Channel;
    class Poller;

    // EventLoop 负责管理 Channels 和 Poller，协调它们之间的关系。
    // 每个线程最多一个 EventLoop，它负责阻塞等待 I/O、分发 Channel 回调、执行跨线程投递的函数、驱动定时器
    class EventLoop : public base::Noncopyable
    {
    public:
        EventLoop();
        ~EventLoop();

        // 循环调用 Poller::Poll() 获取就绪事件
        auto Loop() -> void;

        // 一般不会立刻生效
        auto Quit() -> void;

        auto PollReturnTime() const -> std::chrono::steady_clock::time_point { return poll_return_time_; }

        // 让 EventLoop 执行回调函数。
        // 如果被别的 IO 线程调用，cb 会被放到队列并且会调用WakeUp，而不是立刻执行
        auto RunInLoop(std::function<void()> cb) -> void;
        // 
        auto QueueInLoop(std::function<void()> cb) -> void;

        // 运行主 EventLoop 的线程调用该函数，向 eventfd 写入 1，从而触发读事件
        // 对应的次 EventLoop 线程 从 Poll 阻塞处返回，处理 事件
        auto Wakeup() -> void;

        auto UpdateChannel(Channel* channel) -> void;
        auto RemoveChannel(Channel* channel) -> void;
        auto HasChannel(Channel* channel) const -> bool;

        // 判断当前线程是否为 该EventLoop 创建时所在线程。
        // 别的线程可能拿着它的指针来调用它的方法。
        // 线程共享同一个进程地址空间，所以一个线程创建的对象，另一个线程只要有指针，也能调用
        auto IsInLoopThread() const -> bool { return thread_id_ == net::base::CurrentThread::Tid(); }
    private:

        auto HandleRead() -> void; // 处理 wakeupfd_ 可读事件，唤醒 EventLoop 线程执行 pending functors
        auto DoPendingFunctors() -> void;

        // EventLoop 是否正在运行
        std::atomic<bool> looping_;
        // EventLoop 是否退出
        std::atomic<bool> quit_;

        // EventLoop 所在线程的 id
        const pid_t thread_id_;
        // 上次 Poll 返回的时间点
        std::chrono::steady_clock::time_point poll_return_time_;

        std::unique_ptr<Poller> poller_;

        // 让 IO 线程就从 IOmultiplexing 阻塞调用中返回。
        // 处理读事件。
        int wakeup_fd_;
        std::unique_ptr<Channel> wakeup_channel_;

        std::vector<Channel*> active_channels_;

        // 标识当前是否有正在执行的 pending functors
        std::atomic<bool> calling_pending_functors_;
        // 当前 EventLoop 线程待执行的回调函数列表， 由其他线程投递
        std::vector<std::function<void()>> pending_functors_;

        std::mutex mutex_;
    };
}