#include "connection.h"

#include <cstring>
#include <format>

#include "channel.h"
#include "socket.h"

Connection::Connection(EventLoop* loop, Socket* client_sock) : loop_{loop} {
  client_sock_.reset(client_sock);
  client_channel_ = std::make_unique<Channel>(loop_, client_sock_->fd());

  // 设置客户端的读回调
  client_channel_->SetReadCallback([this] { HandleOnMessage(); });
  // 设置写回调
  client_channel_->SetWriteCallback([this] { WriteCallback(); });
  // 设置客户端连接断开的回调
  client_channel_->SetCloseCallback([this] { CloseCallback(); });
  //  设置客户端连接错误的回调
  client_channel_->SetErrorCallback([this] { ErrorCallback(); });

  // 设置边缘触发
  // client_channel_->UseET();
  // 为新客户端连接准备读事件，并添加到epoll中。
  client_channel_->EnableReading();
}

Connection::~Connection() = default;

int Connection::fd() const { return client_sock_->fd(); }
uint16_t Connection::port() const { return client_sock_->port(); }
const std::string& Connection::ip() const { return client_sock_->ip(); }

void Connection::CloseCallback() { close_callback_(this); }

void Connection::ErrorCallback() { error_callback_(this); }

void Connection::HandleOnMessage() {
  char buffer[1024]{};
  // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
  while (true) {
    std::fill(buffer, buffer + sizeof(buffer), 0);
    ssize_t nread = ::read(fd(), buffer, sizeof(buffer));
    if (nread > 0)  // 成功的读取到了数据。
    {
      input_buffer_.Append(buffer, nread);
    } else if (nread == -1 && errno == EINTR) {
      // 读取数据的时候被信号中断，继续读取。
      continue;
    } else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
      // 内核接收缓冲区的全部的数据已读取完毕。
      while (true) {
        int len;
        std::memcpy(&len, input_buffer_.data(), sizeof(len));
        // inputbuffer 中数据量小于len，说明报文不完整
        if (input_buffer_.size() < len + 4) break;

        // 从inputbuffer获取 一个报文
        std::string message{input_buffer_.data() + sizeof(len),
                            static_cast<size_t>(len)};
        input_buffer_.Erase(0, sizeof(len) + len);

        // Calculate...
        std::cout << std::format("message (eventfd={}):{}", fd(), message);

        on_message_callback_(this, message);
      }
      break;
    } else if (nread == 0) {  // 客户端连接已断开。
      close_callback_(this);
      break;
    }
  }
}

void Connection::WriteCallback() {
  // 尝试把output_buffer数据全部发送
  int written = ::send(fd(), output_buffer_.data(), output_buffer_.size(), 0);
  if (written > 0) {
    output_buffer_.Erase(0, written);
  }
  // 数据已经发完，需要取消监听可写事件
  if (output_buffer_.size() == 0) {
    client_channel_->DisableWriting();
    send_complete_callback_(this);
  }
}

void Connection::Send(const char* data, size_t len) {
  output_buffer_.Append(data, len);  // 将用户数据 保存到发送缓冲区
  client_channel_->EnableWriting();  // 注册 写事件
}