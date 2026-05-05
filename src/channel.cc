#include "channel.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "connection.h"
#include "epoll.h"
#include "event_loop.h"

Channel::Channel(EventLoop* loop, int fd) : loop_{loop}, fd_{fd} {}

Channel::~Channel() = default;

void Channel::UseET() { events_ |= EPOLLET; }

void Channel::EnableReading() {
  events_ |= EPOLLIN;
  loop_->UpdateChannel(this);
}

void Channel::EnableWriting() {
  events_ |= EPOLLOUT;
  loop_->UpdateChannel(this);
}

void Channel::SetInEpoll() { in_epoll_ = true; }

void Channel::SetRevents(uint32_t e) { revents_ = e; }

void Channel::HandleEvent() {
  if (revents_ & EPOLLRDHUP) {
    close_callback_();
  } else if (revents_ & (EPOLLIN | EPOLLPRI)) {
    //  普通数据  带外数据
    // 接收缓冲区中有数据可以读。
    read_callback_();
  } else if (revents_ & EPOLLOUT) {
    // 有数据需要写
  } else {  // 其它事件，都视为错误。
    error_callback_();
  }
}

// void Channel::HandleNewConnection(Socket* server_sock) {
//   sockaddr_in addr{};
//   InetAddress client_addr;

//   auto client_sock = new Socket{server_sock->Accept(client_addr)};
//   int client_fd = client_sock->fd();

//   std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n",
//                            client_fd, client_addr.ip(), client_addr.port());

//   Connection* conn = new Connection{loop_, client_sock};
// }