#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "event_loop.h"

class Acceptor;
class Socket;
class Connection;
class EventLoop;

class TcpServer {
 public:
  TcpServer(const std::string& ip, uint16_t port);
  ~TcpServer();

  void Start();

  // 客户端连接时，Acceptor会回调该函数
  void HandleNewConnection(Socket* client_sock);


  // 关闭客户端的连接，在Connection中回调
  void HandleCloseConnection(Connection* conn);

  // 处理客户端连接错误，在Connection中回调
  void HandleErrorConnection(Connection* conn);

  // 处理客户端的请求报文，在Connection类中回调
  void OnMessage(Connection* conn, std::string& message);

  // 数据发送完成后，在Connection类中回调
  void SendComplete(Connection* conn);

  // epoll_wait超时后的回调，由EventLoop调用
  void EpollTimeout(EventLoop* loop);

  // for echo server: 设置各类事件回调
  void SetNewConnectionCallback(std::function<void(Connection*)> cb) {
    new_connection_callback_ = std::move(cb);
  }

  void SetCloseConnectionCallback(std::function<void(Connection*)> cb) {
    close_connection_callback_ = std::move(cb);
  }

  void SetErrorConnectionCallback(std::function<void(Connection*)> cb) {
    error_connection_callback_ = std::move(cb);
  }

  void SetOnMessageCallback(std::function<void(Connection*, std::string&)> cb) {
    on_message_callback_ = std::move(cb);
  }

  void SetSendCompleteCallback(std::function<void(Connection*)> cb) {
    send_complete_callback_ = std::move(cb);
  }

  void SetEpollTimeoutCallback(std::function<void(EventLoop*)> cb) {
    epoll_timeout_callback_ = std::move(cb);
  }


  

 private:
  EventLoop loop_;
  std::unique_ptr<Acceptor> acceptor_;
  std::unordered_map<int, Connection*> conns_;



  // for echo server
  // 回调 EchoServere::HandleNewConnection()
  std::function<void(Connection*)> new_connection_callback_;
  // 回调 EchoServere::HandleClose()
  std::function<void(Connection*)> close_connection_callback_;
  // 回调 EchoServere::HandleError()
  std::function<void(Connection*)> error_connection_callback_;
  // 回调 EchoServere::HandleMessage()
  std::function<void(Connection*, std::string&)> on_message_callback_;
  // 回调 EchoServere::HandleSendComplete()
  std::function<void(Connection*)> send_complete_callback_;
  // 回调 EchoServere::HandleEpollTimeout()
  std::function<void(EventLoop*)> epoll_timeout_callback_;
};