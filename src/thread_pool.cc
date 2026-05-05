#include "thread_pool.h"

#include <iostream>

ThreadPool::ThreadPool(int thread_num) {
  threads_.reserve(thread_num);
  for (int i = 0; i < thread_num; i++) {
    threads_.emplace_back([this](std::stop_token token) {
      std::cout << "worker thread id: " << std::this_thread::get_id() << '\n';

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

        task();
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