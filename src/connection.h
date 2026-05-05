#pragma once

#include <cstdint>
#include <memory>
class EventLoop;
class Socket;
class Channel;

// Channel分为专门处理客户连接的Acceptor 和 处理别的事件的Connection
class Connection {
 public:
  Connection(EventLoop* loop, Socket* client_sock);
  ~Connection();

  int fd() const;
  uint16_t port() const;
  const std::string& ip() const;

  // Tcp连接断开的回调，Channel类检测到断开后 回调
  void CloseCallback();

  // Tcp连接错误的 回调，Channel类检测到错误后 回调
  void ErrorCcallback();

 private:
  // Acceptor对应的事件循环，构造时传入
  EventLoop* loop_{};
  // 服务端用于监听连接的socket
  std::unique_ptr<Socket> client_sock_;
  std::unique_ptr<Channel> client_channel_;
};