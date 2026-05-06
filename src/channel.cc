#include "channel.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include "connection.h"
#include "epoll.h"
#include "event_loop.h"

Channel::Channel(EventLoop* loop, int fd) : loop_{loop}, fd_{fd} {}

Channel::~Channel() = default;

void Channel::UseET() { events_ |= EPOLLET; }

void Channel::EnableReading() {
  events_ |= EPOLLIN | EPOLLRDHUP;
  loop_->UpdateChannel(this);
}

void Channel::DisableReading() {
  events_ &= ~EPOLLIN;
  loop_->UpdateChannel(this);
}

void Channel::EnableWriting() {
  events_ |= EPOLLOUT;
  loop_->UpdateChannel(this);
}

void Channel::DisableWriting() {
  events_ &= ~EPOLLOUT;
  loop_->UpdateChannel(this);
}

void Channel::DisableAll() {
  events_ = 0;
  loop_->UpdateChannel(this);
}

void Channel::RemoveFromEpoll() {
  loop_->RemoveChannel(this);
  in_epoll_ = false;
}
void Channel::SetInEpoll() { in_epoll_ = true; }

void Channel::SetRevents(uint32_t e) { revents_ = e; }

void Channel::HandleEvent() {
  if (revents_ &
      (EPOLLIN |
       EPOLLPRI)) {  // 先处理读，如果对方发送了FIN，还能继续读取数据。
    spdlog::info("Channel::HandleEvent() client(fd={}) read data.", fd());
    //  普通数据  带外数据
    // 接收缓冲区中有数据可以读。
    read_callback_();
  } else if (revents_ & EPOLLOUT) {
    // 有数据需要写, 回调Connection的写事件函数,
    // 将Connection中的output_buffer写入socket
    write_callback_();
  } else if (revents_ & EPOLLRDHUP) {  // client 关闭写端
    spdlog::info("Channel::HandleEvent() client(fd={}) disconnected.", fd());
    // remove
    close_callback_();
  } else {  // 其它事件，都视为错误。
    spdlog::error("Channel::HandleEvent() client(fd={}) error.", fd());

    error_callback_();
  }

  // if (revents_ & EPOLLRDHUP) { // client
  // 关闭写端，这里先处理RDHUP，就不能继续读完数据
  //   spdlog::info("Channel::HandleEvent() client(fd={}) disconnected.", fd());
  //   close_callback_();
  // } else if (revents_ & (EPOLLIN | EPOLLPRI)) {
  //   spdlog::info("Channel::HandleEvent() client(fd={}) read data.", fd());
  //   //  普通数据  带外数据
  //   // 接收缓冲区中有数据可以读。
  //   read_callback_();
  // } else if (revents_ & EPOLLOUT) {
  //   // 有数据需要写, 回调Connection的写事件函数,
  //   // 将Connection中的output_buffer写入socket
  //   write_callback_();
  // } else {  // 其它事件，都视为错误。
  //   spdlog::error("Channel::HandleEvent() client(fd={}) error.", fd());
  //   error_callback_();
  // }
}

// void Channel::HandleNewConnection(Socket* server_sock) {
//   sockaddr_in addr{};
//   InetAddress client_addr;

//   auto client_sock = new Socket{server_sock->Accept(client_addr)};
//   int client_fd = client_sock->fd();

//   spdlog::info("accept client(fd={},ip={},port={}) ok.", client_fd,
//                client_addr.ip(), client_addr.port());

//   Connection* conn = new Connection{loop_, client_sock};
// }
