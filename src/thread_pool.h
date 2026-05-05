#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
class ThreadPool {
 
 public:
  ThreadPool(int thread_num);
  ~ThreadPool();
  
  void AddTask(std::function<void()> task);
 private:
  std::vector<std::jthread> threads_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable_any cond_;
};