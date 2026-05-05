#include "epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

#include "channel.h"

Epoll::Epoll() {
  fd_ = ::epoll_create(1);
  if (fd_ < 0) {
    std::cerr << "epoll_create() failed";
    std::exit(-1);
  }
}

Epoll::~Epoll() { ::close(fd_); }

// void Epoll::AddFd(int fd, uint32_t op) {
//   epoll_event ev{};
//   ev.data.fd = fd;
//   ev.events = EPOLLIN;
//   if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
//     std::cerr << "epoll_ctl() failed";
//     std::exit(-1);
//   }
// }

void Epoll::UpdateChannel(Channel* ch) {
  epoll_event ev{};
  ev.data.ptr = ch;
  ev.events = ch->events();

  int op = ch->in_epoll() ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  if (epoll_ctl(fd_, op, ch->fd(), &ev) == -1) {
    std::cerr << "epoll_ctl() failed";
    std::exit(-1);
  }
  ch->SetInEpoll();
}

// std::vector<epoll_event> Epoll::Loop(int timeout) {
//   std::vector<epoll_event> evs;
//   int n = ::epoll_wait(fd_, events_.data(), events_.size(), timeout);
//   if (n < 0) {
//     std::cerr << "epoll_wait() failed";
//     std::exit(-1);
//   }
//   if (n == 0) {
//     // timeout
//     return evs;
//   }
//   evs.insert(evs.begin(), events_.begin(), events_.begin() + n);
//   return evs;
// }

std::vector<Channel*> Epoll::Loop(int timeout) {
  std::vector<Channel*> chs;
  int n = ::epoll_wait(fd_, events_.data(), events_.size(), timeout);
  if (n < 0) {
    std::cerr << "epoll_wait() failed";
    std::exit(-1);
  }
  if (n == 0) {
    // timeout
    return chs;
  }

  chs.reserve(n);
  for (int i = 0; i < n; i++) {
    auto ch = reinterpret_cast<Channel*>(events_[i].data.ptr);
    // Update revents
    ch->SetRevents(events_[i].events);
    chs.push_back(ch);
  }
  return chs;
}