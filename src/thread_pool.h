#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
class ThreadPool {
 public:
  ThreadPool(int thread_num, std::string thread_type);
  ~ThreadPool();

  void AddTask(std::function<void()> task);

  size_t Size() const { return threads_.size(); }

 private:
  std::vector<std::jthread> threads_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable_any cond_;
  std::string thread_type_;  // IO  / WORK
};