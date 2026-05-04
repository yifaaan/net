#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace net
{
    /** 區分線程池用途：Io 為 reactor / I-O 綁定循環；Worker 為業務計算型任務。 */
    enum class ThreadPoolKind
    {
        Io,
        Worker
    };

    class ThreadPool
    {
    public:
        ThreadPool(ThreadPoolKind kind, size_t num_threads);

        ThreadPoolKind kind() const { return kind_; }

        ~ThreadPool();

        void AddTask(std::function<void()> task);

        void Stop();
    private:
        ThreadPoolKind kind_;
        std::vector<std::jthread> threads_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable condition_variable_;
    };
}