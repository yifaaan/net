#include "timer.h"

namespace net
{
    Timer::Timer(std::function<void()> callback, std::chrono::steady_clock::time_point expiration, std::chrono::steady_clock::duration interval)
        : callback_(std::move(callback)), expiration_(expiration), interval_(interval), sequence_(num_created_++)
    {
    }

    void Timer::Run() const
    {
        if (callback_)
        {
            callback_();
        }
    }

    void Timer::Restart(std::chrono::steady_clock::time_point now) noexcept
    {
        if (Repeat())
        {
            expiration_ = now + interval_;
        }
    }

    uint64_t Timer::NumCreated() noexcept
    {
        return num_created_.load(std::memory_order_relaxed);
    }
}