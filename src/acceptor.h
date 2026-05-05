#pragma once

#include <cstdint>
#include <functional>
#include <memory>
class EventLoop;
class Socket;
class Channel;

// Channel分为专门处理客户连接的Acceptor 和 处理别的事件的Connection
class Acceptor {
 public:
  Acceptor(EventLoop* loop, const std::string& ip, uint16_t port);

  // Acceptor处理客户端新连接事件，并创建 Connection类
  void HandleNewConnection();

  void SetNewConnectionCallback(std::function<void(Socket*)> cb) {
    new_connection_callback_ = std::move(cb);
  }

 private:
  // Acceptor对应的事件循环，构造时传入
  EventLoop* loop_{};
  // 服务端用于监听连接的socket
  std::unique_ptr<Socket> srv_sock_;
  std::unique_ptr<Channel> accept_channel_;

  // 处理客户端连接的回调，有TcpServer创建Acceptor时设置，调用TcpServer::HandleNewConnection
  std::function<void(Socket*)> new_connection_callback_;
};