#pragma once

#include <functional>
#include <optional>
#include <string>
#include <thread>

#include "noncopyable.h"

namespace net::base
{
    class Thread : Noncopyable
    {
    public:
        explicit Thread(std::function<void()>, std::string name = "");
        ~Thread();

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        auto start() -> void;
        auto join() -> void;

        auto started() const noexcept -> bool { return started_; }
        auto tid() const noexcept -> pid_t { return tid_; }
        auto name() const noexcept -> const std::string& { return name_; }
        static auto num_created() noexcept -> int { return num_created_; }

    private:
        auto SetDefaultName() -> void;

        inline static std::atomic<int> num_created_{ 0 };

        bool started_;
        bool joined_;
        pid_t tid_;
        std::string name_;
        std::function<void()> func_;
        std::optional<std::jthread> jthread_;
    };
}