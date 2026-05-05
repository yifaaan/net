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
 
 private:
  // Acceptor对应的事件循环，构造时传入
  EventLoop* loop_{};
  // 服务端用于监听连接的socket
  std::unique_ptr<Socket> client_sock_;
  std::unique_ptr<Channel> client_channel_;
};