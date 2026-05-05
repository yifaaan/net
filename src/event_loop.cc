#include "event_loop.h"

#include "channel.h"
#include "epoll.h"

EventLoop::EventLoop() : epoll_{std::make_unique<Epoll>()} {}

EventLoop::~EventLoop() = default;

void EventLoop::UpdateChannel(Channel* ch) { epoll_->UpdateChannel(ch); }

void EventLoop::Run() {
  while (true) {
    // epoll_wait
    auto channels = epoll_->Loop();

    for (auto ch : channels) {
      ch->HandleEvent();
    }
  }
}