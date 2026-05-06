#pragma once

#include <unistd.h>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "channel.h"
#include "epoll.h"

class Channel;

// 事件循环，while总调用epoll_wait
class EventLoop {
 public:
  EventLoop();
  ~EventLoop();

  Epoll* epoll() { return &epoll_; }

  void Run();

  void UpdateChannel(Channel* ch);
  void RemoveChannel(Channel* ch);

  void SetEpollTimeoutCallback(std::function<void(EventLoop*)> cb) {
    epoll_timeout_callback_ = std::move(cb);
  }

  
  // 唤醒该事件循环
  void Wakeup();

  void HandleWakeup();

  // 判断当前线程是否是运行事件循环的线程
  bool IsInLoopThread() const {
    return thread_id_ == std::this_thread::get_id();
  }

  // 向待执行队列添加任务
  void QueueInLoop(std::function<void()> task); 

  // 执行队列的任务
  void DoPendingTasks();

 private:
  Epoll epoll_;
  int wakeup_fd_{-1};
  Channel wakeup_channel_;
  std::thread::id thread_id_;

  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  // epoll_wait超时后，调用的回调-> TcpServer::EpollTimeout()
  std::function<void(EventLoop*)> epoll_timeout_callback_;
};