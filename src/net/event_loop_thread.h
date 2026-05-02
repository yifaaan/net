#pragma once

#include <condition_variable>
#include <mutex>

#include "../base/noncopyable.h"
#include "../base/thread.h"

namespace net
{
    class EventLoop;

    class EventLoopThread : public base::Noncopyable
    {
    public:
        EventLoopThread(std::function<void(EventLoop*)> cb, std::string name);
        ~EventLoopThread();

        // 主线程调用 
        EventLoop* StartLoop();

    private:
        // 在子线程函数里运行，创建了 EventLoop
        void ThreadFunc();

        base::Thread thread_;
        std::mutex mutex_;
        bool exiting_;
        EventLoop* loop_;
        std::condition_variable cond_;
        std::function<void(EventLoop*)> thread_init_callback_;
    };
}