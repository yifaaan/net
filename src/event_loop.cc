#include "event_loop.h"

#include <iostream>
#include <thread>

#include "channel.h"
#include "epoll.h"

EventLoop::EventLoop() : epoll_{std::make_unique<Epoll>()} {}

EventLoop::~EventLoop() = default;

void EventLoop::UpdateChannel(Channel* ch) { epoll_->UpdateChannel(ch); }

void EventLoop::Run() {
  // std::cout << "EventLoop::Run() thread is " << std::this_thread::get_id()
  //           << "\n";
  while (true) {
    // epoll_wait
    auto channels = epoll_->Loop(5000);

    // 如果 channels为空，表示epoll_wait超时，回调TcpServer的超时函数
    if (channels.empty()) {
      epoll_timeout_callback_(this);
    }

    for (auto ch : channels) {
      ch->HandleEvent();
    }
  }
}