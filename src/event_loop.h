#pragma once

#include <sys/timerfd.h>
#include <unistd.h>

#include <functional>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include "channel.h"
#include "connection.h"
#include "epoll.h"

class Channel;

// 事件循环，while总调用epoll_wait
class EventLoop {
 public:
  EventLoop(bool is_main_loop, int interval, int timeout);
  ~EventLoop();

  Epoll* epoll() { return &epoll_; }

  void Run();

  // 线程安全：请求退出事件循环（会唤醒 epoll_wait）
  void Quit();

  void UpdateChannel(Channel* ch);
  void RemoveChannel(Channel* ch);

  void SetEpollTimeoutCallback(std::function<void(EventLoop*)> cb) {
    epoll_timeout_callback_ = std::move(cb);
  }

  void SetConnTimeoutCallback(std::function<void(int)> cb) {
    conn_time_out_ = std::move(cb);
  }

  // 唤醒该事件循环
  void Wakeup();

  void HandleWakeup();

  // 定时器超时
  void HandleTimer();

  // 判断当前线程是否是运行事件循环的线程
  bool IsInLoopThread() const {
    return thread_id_ == std::this_thread::get_id();
  }

  // 向待执行队列添加任务
  void QueueInLoop(std::function<void()> task);

  // 执行队列的任务
  void DoPendingTasks();


  void NewConnection(Connection::Ptr conn);

  void RemoveConnection(int fd);

 private:
  Epoll epoll_;
  int wakeup_fd_{-1};
  Channel wakeup_channel_;
  std::thread::id thread_id_;

  std::atomic_bool quitting_{false};

  int timerfd_{-1};
  Channel timer_channel_;
  bool is_main_loop_{};

  // 当前Loop负责的连接
  std::unordered_map<int, Connection::Ptr> conns_;
  std::mutex conns_mutex_;
  int timeout_{}; // 空闲连接的最大存活时间

  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  // epoll_wait超时后，调用的回调-> TcpServer::EpollTimeout()
  std::function<void(EventLoop*)> epoll_timeout_callback_;

  // 连接的空闲时间过长，需要断开，TcpServer创建Connection时设置
  std::function<void(int)> conn_time_out_;
};