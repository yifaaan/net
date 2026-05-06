#include "event_loop.h"

#include <sys/eventfd.h>

#include "channel.h"
#include "epoll.h"

EventLoop::EventLoop()
    : wakeup_fd_{::eventfd(0, EFD_NONBLOCK)},
      wakeup_channel_{this, wakeup_fd_} {
  wakeup_channel_.SetReadCallback([this] { HandleWakeup(); });
  // 将eventfd加入 epoll，等待唤醒
  wakeup_channel_.UseET();
  wakeup_channel_.DisableReading();
}

EventLoop::~EventLoop() = default;

void EventLoop::UpdateChannel(Channel* ch) { epoll_.UpdateChannel(ch); }

void EventLoop::RemoveChannel(Channel* ch) { epoll_.RemoveChannel(ch); }

void EventLoop::Wakeup() {
  uint64_t one = 1;
  ::write(wakeup_fd_, &one, sizeof(one));
}

void EventLoop::HandleWakeup() {
  uint64_t one;
  ::read(wakeup_fd_, &one, sizeof(one));  // 必须读取，否则在LT模式下会一直触发

  DoPendingTasks();
}

void EventLoop::Run() {
  // std::cout << "EventLoop::Run() thread is " << std::this_thread::get_id()
  //           << "\n";
  thread_id_ = std::this_thread::get_id();
  while (true) {
    // epoll_wait
    auto channels = epoll_.Loop(5000);

    // 如果 channels为空，表示epoll_wait超时，回调TcpServer的超时函数
    if (channels.empty()) {
      epoll_timeout_callback_(this);
    }

    for (auto ch : channels) {
      ch->HandleEvent();
    }
  }
}

void EventLoop::QueueInLoop(std::function<void()> task) {
  {
    std::unique_lock lock{mutex_};
    tasks_.push(std::move(task));
  }
  //  唤醒EventLoop
  Wakeup();
}

void EventLoop::DoPendingTasks() {
  decltype(tasks_) tmp;
  {
    std::unique_lock lock{mutex_};
    tmp.swap(tasks_);
  }
  while (!tmp.empty()) {
    auto t = std::move(tmp.front());
    tmp.pop();
    t();
  }
}