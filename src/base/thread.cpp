#include "thread.h"

#include <format>
#include <future>

#include "current_thread.h"

namespace net::base
{
    Thread::Thread(std::function<void()> func, std::string name) : started_(false), joined_(false), tid_(0), name_(std::move(name)), func_(std::move(func))
    {
        SetDefaultName();
    }

    Thread::~Thread()
    {
        jthread_.reset();
    }

    auto Thread::Start() -> void
    {
        started_ = true;
        // 等待线程，直到被调度运行起来
        std::promise<void> ready;
        std::future<void> future = ready.get_future();

        jthread_.emplace([this, r = std::move(ready)](std::stop_token) mutable {
            tid_ =  CurrentThread::Tid();
            r.set_value(); // 通知主线程，线程已经准备好
            
            // 执行线程函数，上一行就通知主线程了
            func_();
        });
        future.wait();
        // tid_ 已经设置好了，可以安全地访问
    }

    auto Thread::join() -> void
    {
        if (jthread_.has_value())
        {
            jthread_->join();
            jthread_.reset();
        }
        joined_ = true;
    }

    auto Thread::SetDefaultName() -> void
    {
        if (name_.empty())
        {
            name_ = std::format("Thread-{}", ++num_created_);
        }
    }

}