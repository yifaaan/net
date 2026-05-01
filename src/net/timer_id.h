#pragma once

#include <cstdint>

namespace net
{
    class Timer;
    class TimerQueue;

    // TimerId 是用户拿到的“定时器句柄”。
    // 它不拥有 Timer，只是用来告诉 TimerQueue：我要取消哪个 Timer。
    class TimerId
    {
    public:
        TimerId() noexcept = default;

        TimerId(Timer* timer, uint64_t sequence) noexcept : timer_(timer), sequence_(sequence)
        {
        }

        operator bool() const noexcept
        {
            return timer_ != nullptr;
        }

    private:
        friend class TimerQueue;

        Timer* timer_ = nullptr;
        uint64_t sequence_ = 0;
    };
}