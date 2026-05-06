#pragma once

#include <functional>
#include <memory>


class Epoll;
class Channel;

// 事件循环，while总调用epoll_wait
class EventLoop {
 public:
  EventLoop();
  ~EventLoop();

  Epoll* epoll() const { return epoll_.get(); }

  void Run();

  void UpdateChannel(Channel* ch);
  void RemoveChannel(Channel* ch);
  
  void SetEpollTimeoutCallback(std::function<void(EventLoop*)> cb) {
    epoll_timeout_callback_ = std::move(cb);
  }

 private:
  std::unique_ptr<Epoll> epoll_;

  // epoll_wait超时后，调用的回调-> TcpServer::EpollTimeout()
  std::function<void(EventLoop*)> epoll_timeout_callback_;
};