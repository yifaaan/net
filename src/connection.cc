#include "connection.h"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <cstring>

#include "channel.h"
#include "event_loop.h"
#include "log.h"
#include "socket.h"

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> client_sock)
    : loop_{loop}, client_sock_{std::move(client_sock)} {
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
  client_channel_->UseET();
  // 为新客户端连接准备读事件，并添加到epoll中。
  client_channel_->EnableReading();
}

Connection::~Connection() {
  spdlog::info("component=connection event=~connection() fd={}",
               client_sock_->fd());
}

int Connection::fd() const { return client_sock_->fd(); }
uint16_t Connection::port() const { return client_sock_->port(); }
const std::string& Connection::ip() const { return client_sock_->ip(); }

void Connection::CloseCallback() {
  client_channel_->RemoveFromEpoll();
  close_callback_(shared_from_this());
}

void Connection::ErrorCallback() {
  client_channel_->RemoveFromEpoll();
  error_callback_(shared_from_this());
}

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

        spdlog::info(
            "component=connection event=message_ready fd={} bytes={} "
            "message={}",
            fd(), len, message);

        // 更新连接的活跃时间
        last_active_time_ = Clock::now();
        on_message_callback_(shared_from_this(), message);
      }
      break;
    } else if (nread == 0) {  // 客户端发FIN，关闭了写端，继续为0，连接已断开。
      spdlog::info("component=connection event=peer_closed fd={} nread=0",
                   fd());
      close_callback_(shared_from_this());
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
    send_complete_callback_(shared_from_this());
  }
}

void Connection::Send(const char* data, size_t len) {
  spdlog::info("component={} event={}, send_bytes={}", "connection",
               "send_message", len);
  if (loop_->IsInLoopThread()) {
    // 是IO线程，直接发送
    SendInLoop(data, len);
  } else {
    // 将发送操作提交给IO线程去处理
    loop_->QueueInLoop(
        [=, self = shared_from_this()] { self->SendInLoop(data, len); });
  }
}

void Connection::SendInLoop(const char* data, size_t len) {
  output_buffer_.AppendWithHead(data, len);  // 将用户数据 保存到发送缓冲区
  client_channel_->EnableWriting();          // 注册 写事件
}

void Connection::Shutdown() {
  if (loop_->IsInLoopThread()) {
    ShutdownInLoop();
    return;
  }
  loop_->QueueInLoop([self = shared_from_this()] { self->ShutdownInLoop(); });
}

void Connection::ShutdownInLoop() {
  // 在所属 IO 线程关闭：先从 epoll 移除，再关闭 fd
  client_channel_->DisableAll();
  client_channel_->RemoveFromEpoll();
  loop_->RemoveConnection(fd());
  if (client_sock_) {
    client_sock_->Close();
  }
}
