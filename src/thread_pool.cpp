#include "thread_pool.h"

#include <stop_token>

namespace net
{
    ThreadPool::ThreadPool(ThreadPoolKind kind, size_t num_threads) : kind_{ kind }
    {
        threads_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            threads_.emplace_back([this](std::stop_token st) {
                for (;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(mutex_);

                        condition_variable_.wait(lock, [&] {
                            return st.stop_requested() || !tasks_.empty();
                        });

                        if (st.stop_requested() && tasks_.empty())
                        {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                }
            });
        }
    }

    void ThreadPool::AddTask(std::function<void()> task)
    {
        {
            std::unique_lock lock(mutex_);
            tasks_.emplace(std::move(task));
        }
        condition_variable_.notify_one();
    }

    void ThreadPool::Stop()
    {
        for (auto& j : threads_)
        {
            j.request_stop();
        }
        condition_variable_.notify_all();
    }

    ThreadPool::~ThreadPool()
    {
        Stop();
        for (auto& j : threads_)
        {
            if (j.joinable())
            {
                j.join();
            }
        }
    }

}