#include "channel.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <format>
#include <iostream>

#include "epoll.h"
#include "inet_address.h"
#include "socket.h"
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
    // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
    std::cout << std::format("1client(eventfd={}) disconnected.\n", fd_);
    ::close(fd_);  // 关闭客户端的fd。
  } else if (revents_ & (EPOLLIN | EPOLLPRI)) {
    //  普通数据  带外数据
    // 接收缓冲区中有数据可以读。
    read_callback_();
  } else if (revents_ & EPOLLOUT) {
    // 有数据需要写
  } else {  // 其它事件，都视为错误。
    std::cout << std::format("client(eventfd={}) error.\n", fd_);
    ::close(fd_);  // 关闭客户端的fd。
  }
}

void Channel::HandleNewConnection(Socket* server_sock) {
  sockaddr_in addr{};
  InetAddress client_addr;

  auto client_sock = new Socket{server_sock->Accept(client_addr)};
  int client_fd = client_sock->fd();

  std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n",
                           client_fd, client_addr.ip(), client_addr.port());

  // 为新客户端连接准备读事件，并添加到epoll中。
  auto client_ch = new Channel{loop_, client_fd};
  // 指定读回调
  client_ch->SetReadCallback([=] { client_ch->HandleOnMessage(); });
  client_ch->UseET();  // 边缘触发
  client_ch->EnableReading();
}

void Channel::HandleOnMessage() {
  char buffer[1024]{};
  // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
  while (true) {
    ssize_t nread = ::read(fd_, buffer, sizeof(buffer));
    if (nread > 0)  // 成功的读取到了数据。
    {
      // 把接收到的报文内容原封不动的发回去。
      std::cout << std::format(
          "recv(eventfd={}):{}\n", fd_,
          std::string_view(buffer, static_cast<size_t>(nread)));
      ::send(fd_, buffer, static_cast<size_t>(nread), 0);
    } else if (nread == -1 && errno == EINTR) {
      // 读取数据的时候被信号中断，继续读取。
      continue;
    } else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
      // 全部的数据已读取完毕。
      break;
    } else if (nread == 0) {  // 客户端连接已断开。
      std::cout << std::format("client(eventfd={}) disconnected.\n", fd_);
      ::close(fd_);  // 关闭客户端的fd。
      break;
    }
  }
}