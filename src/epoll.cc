#include "epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

Epoll::Epoll() {
  int fd = ::epoll_create(1);
  if (fd < 0) {
    std::cerr << "epoll_create() failed";
    std::exit(-1);
  }
}

Epoll::~Epoll() { ::close(fd_); }

void Epoll::AddFd(int fd, uint32_t op) {
  epoll_event ev{};
  ev.data.fd = fd;
  ev.events = EPOLLIN;
  if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
    std::cerr << "epoll_ctl() failed";
    std::exit(-1);
  }
}

std::vector<epoll_event> Epoll::Loop(int timeout) {
  std::vector<epoll_event> evs;
  int n = ::epoll_wait(fd_, events_.data(), events_.size(), timeout);
  if (n < 0) {
    std::cerr << "epoll_wait() failed";
    std::exit(-1);
  }
  if (n == 0) {
    // timeout
    return evs;
  }
  evs.insert(evs.begin(), events_.begin(), events_.begin() + n);
  return evs;
}