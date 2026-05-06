#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "event_loop.h"
#include "connection.h"

class Acceptor;
class Socket;
class EventLoop;
class ThreadPool;

class TcpServer {
 public:
  TcpServer(const std::string& ip, uint16_t port, int thread_num = 3);
  ~TcpServer();

  void Start();

  // 客户端连接时，Acceptor会回调该函数
  void HandleNewConnection(std::unique_ptr<Socket> client_sock);

  // 关闭客户端的连接，在Connection中回调
  void HandleCloseConnection(Connection::Ptr conn);

  // 处理客户端连接错误，在Connection中回调
  void HandleErrorConnection(Connection::Ptr conn);

  // 处理客户端的请求报文，在Connection类中回调
  void OnMessage(Connection::Ptr conn, std::string& message);

  // 数据发送完成后，在Connection类中回调
  void SendComplete(Connection::Ptr conn);

  // epoll_wait超时后的回调，由EventLoop调用
  void EpollTimeout(EventLoop* loop);

  // 客户连接空闲后的删除回调
  void RemoveConn(int fd);

  // for echo server: 设置各类事件回调
  void SetNewConnectionCallback(std::function<void(Connection::Ptr)> cb) {
    new_connection_callback_ = std::move(cb);
  }

  void SetCloseConnectionCallback(std::function<void(Connection::Ptr)> cb) {
    close_connection_callback_ = std::move(cb);
  }

  void SetErrorConnectionCallback(std::function<void(Connection::Ptr)> cb) {
    error_connection_callback_ = std::move(cb);
  }

  void SetOnMessageCallback(std::function<void(Connection::Ptr, std::string&)> cb) {
    on_message_callback_ = std::move(cb);
  }

  void SetSendCompleteCallback(std::function<void(Connection::Ptr)> cb) {
    send_complete_callback_ = std::move(cb);
  }

  void SetEpollTimeoutCallback(std::function<void(EventLoop*)> cb) {
    epoll_timeout_callback_ = std::move(cb);
  }

 private:
  std::unique_ptr<EventLoop> main_loop_;
  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<int, Connection::Ptr> conns_;
  int thread_num_;  // 从事件循环的个数
  std::vector<std::unique_ptr<EventLoop>> sub_loops_;
  std::unique_ptr<ThreadPool> thread_pool_;

  // for echo server
  // 回调 EchoServere::HandleNewConnection()
  std::function<void(Connection::Ptr)> new_connection_callback_;
  // 回调 EchoServere::HandleClose()
  std::function<void(Connection::Ptr)> close_connection_callback_;
  // 回调 EchoServere::HandleError()
  std::function<void(Connection::Ptr)> error_connection_callback_;
  // 回调 EchoServere::HandleMessage()
  std::function<void(Connection::Ptr, std::string&)> on_message_callback_;
  // 回调 EchoServere::HandleSendComplete()
  std::function<void(Connection::Ptr)> send_complete_callback_;
  // 回调 EchoServere::HandleEpollTimeout()
  std::function<void(EventLoop*)> epoll_timeout_callback_;
};