#include "event_loop_thread.h"

#include "event_loop.h"

namespace net
{
    EventLoopThread::EventLoopThread(std::function<void(EventLoop*)> cb, std::string name) : thread_(std::bind(&EventLoopThread::ThreadFunc, this), std::move(name)),
                                                                                             exiting_(false),
                                                                                             loop_(nullptr),
                                                                                             thread_init_callback_(std::move(cb))
    {
    }

    EventLoopThread::~EventLoopThread()
    {
        exiting_ = true;
        if (loop_)
        {
            loop_->Quit();
            thread_.join();
        }
    }

    EventLoop* EventLoopThread::EventLoopThread::StartLoop()
    {
        thread_.Start();
        {
            std::unique_lock lock(mutex_);
            cond_.wait(lock, [this]() {
                return loop_ != nullptr;
            });
        }
        return loop_;
    }

    void EventLoopThread::ThreadFunc()
    {
        EventLoop loop;
        if (thread_init_callback_)
        {
            thread_init_callback_(&loop);
        }

        {
            std::unique_lock lock(mutex_);
            loop_ = &loop;
            // 主线程等待条件变量，直到子线程创建好了 EventLoop
            cond_.notify_one();
        }
        loop.Loop();
        std::unique_lock lock(mutex_);
        loop_ = nullptr;
    }
}