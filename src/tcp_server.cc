#include "tcp_server.h"

#include <format>

#include "acceptor.h"
#include "channel.h"
#include "connection.h"
#include "socket.h"
#include "thread_pool.h"

TcpServer::TcpServer(const std::string& ip, uint16_t port, int thread_num)
    : main_loop_{std::make_unique<EventLoop>()},
      acceptor_{std::make_unique<Acceptor>(main_loop_.get(), ip, port)},
      thread_num_{thread_num},
      thread_pool_{std::make_unique<ThreadPool>(thread_num_, "IO")} {
  // Acceptor 收到客户连接时，回调TcpServer的函数
  acceptor_->SetNewConnectionCallback(
      [this](std::unique_ptr<Socket> client_sock) { HandleNewConnection(std::move(client_sock)); });

  // 设置epoll_wait超时回调
  main_loop_->SetEpollTimeoutCallback(
      [this](EventLoop* loop) { EpollTimeout(loop); });

  // 创建从事件循环
  sub_loops_.reserve(thread_num_);
  for (int i = 0; i < thread_num_; i++) {
    sub_loops_.emplace_back(std::make_unique<EventLoop>());
    auto lp = sub_loops_[i].get();
    // 超时回调
    lp->SetEpollTimeoutCallback(
        [this](EventLoop* loop) { EpollTimeout(loop); });
    // 添加线程池任务
    thread_pool_->AddTask([lp] { lp->Run(); });
  }
}

TcpServer::~TcpServer() = default;

void TcpServer::Start() { main_loop_->Run(); }

void TcpServer::HandleNewConnection(std::unique_ptr<Socket> client_sock) {
  // TODO:设置到从事件循环中
  int id = client_sock->fd() % thread_num_;
  auto conn = std::make_shared<Connection>(sub_loops_[id].get(), std::move(client_sock));
  conns_[conn->fd()] = conn;
  // 设置事件发生时的回调
  conn->SetCloseCallback(
      [this](Connection::Ptr conn) { HandleCloseConnection(conn); });
  conn->SetErrorCallback(
      [this](Connection::Ptr conn) { HandleErrorConnection(conn); });
  conn->SetOnMessageCallback([this](Connection::Ptr conn, std::string& message) {
    OnMessage(conn, message);
  });
  conn->SetSendCompeleteCallbace(
      [this](Connection::Ptr conn) { SendComplete(conn); });
  std::cout << std::format("TcpServer::HandleNewConnection() client(fd={},ip={},port={}) ok.\n",
                           conn->fd(), conn->ip(), conn->port());

  // for echo server
  new_connection_callback_(conn);
}

void TcpServer::HandleCloseConnection(Connection::Ptr conn) {
  std::cout << std::format("TcpServer::HandleCloseConnection() client(fd={}) disconnected.\n", conn->fd());
  // for echo server
  close_connection_callback_(conn);

  // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
  // std::cout << std::format("1client(eventfd={}) disconnected.\n",
  // conn->fd());
  conns_.erase(conn->fd());
}

void TcpServer::HandleErrorConnection(Connection::Ptr conn) {
  // for echo server
  error_connection_callback_(conn);

  // std::cout << std::format("client(eventfd={}) error.\n", conn->fd());
  conns_.erase(conn->fd());
}

void TcpServer::OnMessage(Connection::Ptr conn, std::string& message) {
  // for echo server
  on_message_callback_(conn, message);

  // message = "reply:" + message;

  // // 回应报文
  // int len = message.size();
  // std::string tmp{reinterpret_cast<char*>(&len), sizeof(len)};
  // tmp += message;
  // // 将数据写入Connection的output_buffer
  // conn->Send(tmp.data(), tmp.size());
}

void TcpServer::SendComplete(Connection::Ptr conn) {
  // for echo server
  send_complete_callback_(conn);

  // ...
}

void TcpServer::EpollTimeout(EventLoop* loop) {
  // for echo server
  epoll_timeout_callback_(loop);

  // ...
}