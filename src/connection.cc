#include "connection.h"

#include "channel.h"
#include "socket.h"

Connection::Connection(EventLoop* loop, Socket* client_sock) : loop_{loop} {
  client_sock_.reset(client_sock);
  client_channel_ = std::make_unique<Channel>(loop_, client_sock_->fd());

  // 设置客户端的读回调
  client_channel_->SetReadCallback(
      [ch = client_channel_.get()] { ch->HandleOnMessage(); });
  client_channel_->UseET();
  // 为新客户端连接准备读事件，并添加到epoll中。
  client_channel_->EnableReading();
}

Connection::~Connection() = default;

int Connection::fd() const { return client_sock_->fd(); }
uint16_t Connection::port() const { return client_sock_->port(); }
const std::string& Connection::ip() const { return client_sock_->ip(); }