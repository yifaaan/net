#include "thread_pool.h"

#include <thread>

#include "log.h"

ThreadPool::ThreadPool(int thread_num, std::string thread_type)
    : thread_type_{std::move(thread_type)} {
  threads_.reserve(thread_num);
  for (int i = 0; i < thread_num; i++) {
    threads_.emplace_back([this](std::stop_token token) {
      spdlog::info("component=thread_pool event=worker_start pool={}",
                   thread_type_);

      while (true) {
        std::unique_lock lock{mutex_};
        // 使用 wait(lock, stop_token, pred)，收到 stop 请求会自动唤醒。
        cond_.wait(lock, token, [this] { return !tasks_.empty(); });
        if (token.stop_requested() || tasks_.empty()) {
          break;
        }
        auto task = std::move(tasks_.front());
        tasks_.pop();

        lock.unlock();

        spdlog::info("component=thread_pool event=task_start pool={}",
                     thread_type_);
        task();
        spdlog::info("component=thread_pool event=task_done pool={}",
                     thread_type_);
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  for (auto& t : threads_) {
    t.request_stop();  // 唤醒 wait处的线程
  }

  for (auto& t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void ThreadPool::AddTask(std::function<void()> task) {
  {
    std::unique_lock lock{mutex_};
    tasks_.push(std::move(task));
  }
  cond_.notify_one();
}
