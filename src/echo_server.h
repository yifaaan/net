#pragma once

#include "tcp_server.h"
class EchoServer {
 public:
  EchoServer(const std::string& ip, uint16_t port, int thread_num, int work_thread_num = 5);
  ~EchoServer();

  void Start();

  // 客户端连接时，TcpServer会回调该函数
  void HandleNewConnection(Connection* conn);


  // 关闭客户端的连接，在Connection中回调
  void HandleClose(Connection* conn);

  // 处理客户端连接错误，在Connection中回调
  void HandleError(Connection* conn);

  // 处理客户端的请求报文，在Connection类中回调
  void HandleMessage(Connection* conn, std::string& message);

  // 数据发送完成后，在Connection类中回调
  void HandleSendComplete(Connection* conn);

  // epoll_wait超时后的回调，在EventLoop类中回调
  void HandleEpollTimeout(EventLoop* loop);


  // 给 工作线程的任务函数
  void OnMessage(Connection* conn, std::string& message);
 private:
  TcpServer tcp_server_;
  int work_thread_num_{};
  std::unique_ptr<ThreadPool>  thread_pool_;
};