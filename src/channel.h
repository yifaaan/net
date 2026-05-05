#pragma once

#include <cstdint>
#include <functional>

class Epoll;
class Socket;
class EventLoop;

// 作为epoll_event.data.ptr，和 Epoll.Loop(epoll_wait)的返回
// 包含fd及其事件控制，epoll_wait返回后处理各种事件
class Channel {
 public:
  Channel(EventLoop* loop, int fd);
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
  void HandleEvent();

  // 处理客户端新连接事件
  // void HandleNewConnection(Socket* server_sock);

  void SetReadCallback(std::function<void()> cb) {
    read_callback_ = std::move(cb);
  }

  void SetWriteCallback(std::function<void()> cb) {
    write_callback_ = std::move(cb);
  }

  void SetCloseCallback(std::function<void()> cb) {
    close_callback_ = std::move(cb);
  }

  void SetErrorCallback(std::function<void()> cb) {
    error_callback_ = std::move(cb);
  }

 private:
  int fd_{-1};
  bool in_epoll_{};     // 已添加到epoll
  EventLoop* loop_{};   // 所属的EventLoop
  uint32_t events_{};   // fd_需要监听的事件
  uint32_t revents_{};  // fd_已发生的事件

  // 读回调，Connection在创建Channel时需要指定
  std::function<void()> read_callback_;
  // 写回调，Connection在创建Channel时需要指定
  std::function<void()> write_callback_;

  // 客户端断开连接的回调，Connection在创建Channel时需要指定
  std::function<void()> close_callback_;
  // 客户端连接错误的回调，Connection在创建Channel时需要指定
  std::function<void()> error_callback_;
};