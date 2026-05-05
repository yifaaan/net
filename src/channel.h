#pragma once

#include <cstdint>

class Epoll;
class Socket;

// 作为epoll_event.data.ptr，和 Epoll.Loop(epoll_wait)的返回
// 包含fd及其事件控制，epoll_wait返回后处理各种事件
class Channel {
 public:
  Channel(Epoll* ep, int fd, bool is_listen);
  ~Channel();

  int fd() const { return fd_; }
  bool in_epoll() const { return in_epoll_; }
  uint32_t events() const { return events_; }
  uint32_t revents() const { return revents_; }

  // 设置边缘触发
  void UseET();

  // 监听读事件，并添加到对应的epoll
  void EnableReading();
  // 监听写事件，并添加到对应的epoll
  void EnableWriting();

  // 表示已添加到epoll
  void SetInEpoll();

  void SetRevents(uint32_t e);

  // epoll_wait返回后，执行该函数
  void HandleEvent(Socket* server_sock);
 private:
  int fd_{-1};
  bool is_listen_{};    // listen fd / client fd?
  bool in_epoll_{};     // 已添加到epoll
  Epoll* ep_{};         // 所属的epoll
  uint32_t events_{};   // fd_需要监听的事件
  uint32_t revents_{};  // fd_已发生的事件
};