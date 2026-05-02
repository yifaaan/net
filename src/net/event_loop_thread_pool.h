#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "../base/noncopyable.h"

namespace net
{
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool : public base::Noncopyable
    {
    public:
        EventLoopThreadPool(EventLoop* baseloop, std::string name);
        ~EventLoopThreadPool() = default;

        auto Start(std::function<void(EventLoop*)> cb) -> void;

        auto SetThreadNum(int n) -> void { num_threads_ = n; }

        auto GetNextLoop() -> EventLoop*;

        auto GetAllLoops() -> std::vector<EventLoop*>;

        auto started() -> bool { return started_; }
        auto name() -> std::string { return name_;  }
    private:
        // 主 EventLoop，通常负责监听 socket / accept
        EventLoop* base_loop_; 
        std::string name_;
        bool started_;
        // 子 IO 线程数量
        int num_threads_;
        // round-robin 分配连接时用的下标
        int next_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;
    };
}