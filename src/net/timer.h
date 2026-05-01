#pragma once

#include <atomic>
#include <chrono>
#include <functional>

#include "../base/noncopyable.h"

namespace net
{
    // Timer 表示一个定时任务。
    // 只保存：什么时候触发、触发时执行什么函数、是否重复。
    class Timer : public base::Noncopyable
    {
    public:
        Timer(std::function<void()> callback, std::chrono::steady_clock::time_point expiration, std::chrono::steady_clock::duration interval = std::chrono::steady_clock::duration::zero());

        // 执行回调函数。
        void Run() const;

        // 返回触发时间。
        std::chrono::steady_clock::time_point Expiration() const noexcept
        {
            return expiration_;
        }

        // 是否重复。
        bool Repeat() const noexcept
        {
            return interval_ > std::chrono::steady_clock::duration::zero();
        }

        // 返回序列号。
        uint64_t Sequence() const noexcept
        {
            return sequence_;
        }

        // 重启定时器。
        void Restart(std::chrono::steady_clock::time_point now) noexcept;

        // 返回创建的定时器数量。
        static uint64_t NumCreated() noexcept;

    private:
        inline static std::atomic<uint64_t> num_created_ = 0;

        std::function<void()> callback_;
        std::chrono::steady_clock::time_point expiration_;
        std::chrono::steady_clock::duration interval_;
        uint64_t sequence_;
    };
}