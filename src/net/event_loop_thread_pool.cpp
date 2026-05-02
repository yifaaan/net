#include "event_loop_thread_pool.h"

#include <format>

#include "event_loop_thread.h"

namespace net
{
    EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, std::string name) : base_loop_(baseloop), name_(std::move(name)), started_(false), num_threads_(0), next_(0)
    {
    }

    auto EventLoopThreadPool::Start(std::function<void(EventLoop*)> cb) -> void
    {
        started_ = true;
        for (int i = 0; i < num_threads_; i++)
        {
            auto name = std::format("{}-{}", name_, i);
            threads_.emplace_back(std::make_unique<EventLoopThread>(cb, name));
            loops_.emplace_back(threads_.back()->StartLoop());
        }
        if (num_threads_ == 0 && cb)
        {
            cb(base_loop_);
        }
    }

    auto EventLoopThreadPool::GetNextLoop() -> EventLoop*
    {
        auto loop = base_loop_;

        if (!loops_.empty())
        {
            loop = loops_[next_++];
            if (static_cast<size_t>(next_) >= loops_.size())
            {
                next_ = 0;
            }
        }
        return loop;
    }

    auto EventLoopThreadPool::GetAllLoops() -> std::vector<EventLoop*>
    {
        if (loops_.empty())
        {
            return { base_loop_ };
        }
        return loops_;
    }
}