#pragma once

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



 private:
  std::unique_ptr<Epoll> epoll_;
};