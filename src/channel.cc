#include "channel.h"

#include <sys/epoll.h>

#include "epoll.h"

Channel::Channel(Epoll* ep, int fd) : ep_{ep}, fd_{fd} {}

Channel::~Channel() = default;

void Channel::UseET() { events_ |= EPOLLET; }

void Channel::EnableReading() {
  events_ |= EPOLLIN;
  ep_->UpdateChannel(this);
}

void Channel::EnableWriting() {
  events_ |= EPOLLOUT;
  ep_->UpdateChannel(this);
}

void Channel::SetInEpoll() { in_epoll_ = true; }

void Channel::SetRevents(uint32_t e) { revents_ = e; }