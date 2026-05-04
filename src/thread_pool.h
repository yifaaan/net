#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace net
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t num_threads);
        ~ThreadPool();

        void AddTask(std::function<void()> task);

        void Stop();
    private:
        std::vector<std::jthread> threads_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable condition_variable_;
    };
}