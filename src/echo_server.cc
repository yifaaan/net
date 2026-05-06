#include "echo_server.h"

#include <format>
#include <iostream>
#include <thread>

#include "connection.h"
#include "tcp_server.h"
#include "thread_pool.h"

EchoServer::EchoServer(const std::string& ip, uint16_t port, int thread_num,
                       int work_thread_num)
    : tcp_server_{ip, port, thread_num},
      work_thread_num_{work_thread_num},
      thread_pool_{std::make_unique<ThreadPool>(work_thread_num_, "WORK")} {
  tcp_server_.SetNewConnectionCallback(
      [this](Connection::Ptr conn) { HandleNewConnection(conn); });
  tcp_server_.SetCloseConnectionCallback(
      [this](Connection::Ptr conn) { HandleClose(conn); });
  tcp_server_.SetErrorConnectionCallback(
      [this](Connection::Ptr conn) { HandleError(conn); });
  tcp_server_.SetOnMessageCallback(
      [this](Connection::Ptr conn, std::string& msg) {
        HandleMessage(conn, msg);
      });
  tcp_server_.SetSendCompleteCallback(
      [this](Connection::Ptr conn) { HandleSendComplete(conn); });
  tcp_server_.SetEpollTimeoutCallback(
      [this](EventLoop* loop) { HandleEpollTimeout(loop); });
}

EchoServer::~EchoServer() = default;

void EchoServer::Start() { tcp_server_.Start(); }

// 客户端连接时，TcpServer会回调该函数
void EchoServer::HandleNewConnection(Connection::Ptr conn) {
  std::cout << "EchoServer::HandleNewConnection() client(fd={}) connected.\n",
      conn->fd();
}

// 关闭客户端的连接，在Connection中回调
void EchoServer::HandleClose(Connection::Ptr conn) {
  std::cout << "EchoServer::HandleClose() client(fd={}) disconnected.\n",
      conn->fd();
}

// 处理客户端连接错误，在Connection中回调
void EchoServer::HandleError(Connection::Ptr conn) {
  std::cout << "connection error\n";
}

// 处理客户端的一条完整 请求报文，在Connection类中回调
void EchoServer::HandleMessage(Connection::Ptr conn, std::string& message) {
  std::cout << "EchoServer::HandleMessage() thread id is "
            << std::this_thread::get_id() << '\n';
  message = "reply:" + message;

  std::weak_ptr<Connection> weak_conn = conn;
  if (thread_pool_->Size() == 0) {  // 没有worker线程，直接在当前IO线程处理
    OnMessage(weak_conn, std::string(message));
    return;
  }

  thread_pool_->AddTask([this, weak_conn, message] {  //  发给worker线程
    // std::cout << "thread id is " << std::this_thread::get_id() << "\n";
    OnMessage(weak_conn, message);
  });
}

// 数据发送完成后，在Connection类中回调
void EchoServer::HandleSendComplete(Connection::Ptr conn) {
  std::cout << std::format(
      "EchoServer::HandleSendComplete() client(fd={}) message send complete.\n",
      conn->fd());
}

// epoll_wait超时后的回调，在EventLoop类中回调
void EchoServer::HandleEpollTimeout(EventLoop* loop) {
  std::cout << "epoll timeout\n";
}

void EchoServer::OnMessage(std::weak_ptr<Connection> conn,
                           const std::string& message) {
  auto c = conn.lock();
  if (!c) {  // 连接已断开
    return;
  }
  c->Send(message.data(), message.size());
}