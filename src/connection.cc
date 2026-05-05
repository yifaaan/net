#include "connection.h"

#include <format>

#include "channel.h"
#include "socket.h"

Connection::Connection(EventLoop* loop, Socket* client_sock) : loop_{loop} {
  client_sock_.reset(client_sock);
  client_channel_ = std::make_unique<Channel>(loop_, client_sock_->fd());

  // 设置客户端的读回调
  client_channel_->SetReadCallback(
      [ch = client_channel_.get()] { ch->HandleOnMessage(); });
  // 设置客户端连接断开的回调
  client_channel_->SetCloseCallback([this] { CloseCallback(); });
  //  设置客户端连接错误的回调
  client_channel_->SetErrorCallback([this] { ErrorCcallback(); });

  // 设置边缘触发
  client_channel_->UseET();
  // 为新客户端连接准备读事件，并添加到epoll中。
  client_channel_->EnableReading();
}

Connection::~Connection() = default;

int Connection::fd() const { return client_sock_->fd(); }
uint16_t Connection::port() const { return client_sock_->port(); }
const std::string& Connection::ip() const { return client_sock_->ip(); }

void Connection::CloseCallback() {
  // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
  std::cout << std::format("1client(eventfd={}) disconnected.\n", fd());
  ::close(fd());  // 关闭客户端的fd。
}

void Connection::ErrorCcallback() {
  std::cout << std::format("client(eventfd={}) error.\n", fd());
  ::close(fd());  // 关闭客户端的fd。
}