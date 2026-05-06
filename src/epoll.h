#pragma once

#include <sys/epoll.h>

#include <array>
#include <vector>

class Channel;

class Epoll {
 public:
  Epoll();
  ~Epoll();

  // 添加fd和监听事件
  // void AddFd(int fd, uint32_t op);

  // 将channel添加或更新到epoll树上
  void UpdateChannel(Channel* ch);

  // 从epoll树删除channel
  void RemoveChannel(Channel* ch);

  // epoll_wait，返回就绪事件
  // std::vector<epoll_event> Loop(int timeout = -1);

  std::vector<Channel*> Loop(int timeout = -1);

 private:
  static constexpr auto kMaxEvents = 1024;
  // epoll 文件描述符
  int fd_{-1};
  std::array<epoll_event, kMaxEvents> events_{};
};