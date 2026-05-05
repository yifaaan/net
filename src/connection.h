#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "buffer.h"

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

  // Tcp连接断开的回调，由Channel类检测到断开后 回调
  void CloseCallback();

  // Tcp连接错误的 回调，由Channel类检测到错误后 回调
  void ErrorCallback();

  // 处理读写事件
  void HandleOnMessage();

  // 设置断开连接的回调，TcpServer调用
  void SetCloseCallback(std::function<void(Connection*)> cb) {
    close_callback_ = std::move(cb);
  }
  // 设置连接错误的回调，TcpServer调用
  void SetErrorCallback(std::function<void(Connection*)> cb) {
    error_callback_ = std::move(cb);
  }
  // 设置收到客户端完整报文的回调，TcpServer调用
  void SetOnMessageCallback(std::function<void(Connection*, std::string&)> cb) {
    on_message_callback_ = std::move(cb);
  }

 private:
  // Acceptor对应的事件循环，构造时传入
  EventLoop* loop_{};
  // 服务端用于监听连接的socket
  std::unique_ptr<Socket> client_sock_;
  std::unique_ptr<Channel> client_channel_;

  Buffer input_buffer_;
  Buffer output_buffer_;

  // 客户端断开连接的回调，TcpServer在创建Connection时需要指定
  std::function<void(Connection*)> close_callback_;
  // 客户端连接错误的回调，TcpServer在创建Connection时需要指定
  std::function<void(Connection*)> error_callback_;
  // 收到一个完整客户端报文的回调，TcpServer在创建Connection时需要指定
  std::function<void(Connection*, std::string&)> on_message_callback_;
};