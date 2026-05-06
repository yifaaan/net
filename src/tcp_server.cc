#include "tcp_server.h"

#include "acceptor.h"
#include "channel.h"
#include "connection.h"
#include "log.h"
#include "socket.h"
#include "thread_pool.h"

TcpServer::TcpServer(const std::string& ip, uint16_t port, int thread_num)
    : main_loop_{std::make_unique<EventLoop>(true)},
      acceptor_{std::make_unique<Acceptor>(main_loop_.get(), ip, port)},
      thread_num_{thread_num},
      thread_pool_{std::make_unique<ThreadPool>(thread_num_, "IO")} {
  // Acceptor 收到客户连接时，回调TcpServer的函数
  acceptor_->SetNewConnectionCallback(
      [this](std::unique_ptr<Socket> client_sock) {
        HandleNewConnection(std::move(client_sock));
      });

  // 设置epoll_wait超时回调
  main_loop_->SetEpollTimeoutCallback(
      [this](EventLoop* loop) { EpollTimeout(loop); });

  // 创建从事件循环
  sub_loops_.reserve(thread_num_);
  for (int i = 0; i < thread_num_; i++) {
    sub_loops_.emplace_back(std::make_unique<EventLoop>(false));
    auto lp = sub_loops_[i].get();
    // 超时回调
    lp->SetEpollTimeoutCallback(
        [this](EventLoop* loop) { EpollTimeout(loop); });

    // 空闲连接删除的回调
    lp->SetConnTimeoutCallback([this](int fd) { RemoveConn(fd); });
    // 添加线程池任务
    thread_pool_->AddTask([lp] { lp->Run(); });
  }
}

TcpServer::~TcpServer() = default;

void TcpServer::Start() { main_loop_->Run(); }

void TcpServer::HandleNewConnection(std::unique_ptr<Socket> client_sock) {
  // TODO:设置到从事件循环中
  int id = client_sock->fd() % thread_num_;
  auto conn = std::make_shared<Connection>(sub_loops_[id].get(),
                                           std::move(client_sock));
  conns_[conn->fd()] = conn;
  // 添加到sub_loop用来清除空闲连接
  sub_loops_[id]->NewConnection(conn);

  // 设置事件发生时的回调
  conn->SetCloseCallback(
      [this](Connection::Ptr conn) { HandleCloseConnection(conn); });
  conn->SetErrorCallback(
      [this](Connection::Ptr conn) { HandleErrorConnection(conn); });
  conn->SetOnMessageCallback(
      [this](Connection::Ptr conn, std::string& message) {
        OnMessage(conn, message);
      });
  conn->SetSendCompeleteCallbace(
      [this](Connection::Ptr conn) { SendComplete(conn); });
  spdlog::info(
      "component=tcp_server event=connection_accept fd={} ip={} port={} "
      "loop_index={}",
      conn->fd(), conn->ip(), conn->port(), id);

  // for echo server
  new_connection_callback_(conn);
}

void TcpServer::HandleCloseConnection(Connection::Ptr conn) {
  spdlog::info("component=tcp_server event=connection_close fd={}", conn->fd());
  // for echo server
  close_connection_callback_(conn);

  // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
  // spdlog::info("component=tcp_server event=connection_close_detected fd={}",
  // conn->fd());
  conns_.erase(conn->fd());
}

void TcpServer::HandleErrorConnection(Connection::Ptr conn) {
  // for echo server
  error_connection_callback_(conn);

  // spdlog::info("component=tcp_server event=connection_error fd={}",
  // conn->fd());
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

void TcpServer::RemoveConn(int fd) { conns_.erase(fd); }
