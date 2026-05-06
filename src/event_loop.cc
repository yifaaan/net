#include "event_loop.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#include <chrono>
#include <ctime>

#include "channel.h"
#include "connection.h"
#include "epoll.h"

namespace {
int CreateTimerFd(int second = 5) {
  int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd < 0) {
    std::exit(-1);
  }
  itimerspec timeout{};
  timeout.it_value.tv_sec = second;
  timeout.it_interval.tv_sec = second;
  if (timerfd_settime(fd, 0, &timeout, nullptr) < 0) {
    ::close(fd);
    std::exit(-1);
  }
  return fd;
}
}  // namespace

EventLoop::EventLoop(bool is_main_loop)
    : wakeup_fd_{::eventfd(0, EFD_NONBLOCK)},
      wakeup_channel_{this, wakeup_fd_},
      timerfd_{CreateTimerFd()},
      timer_channel_{this, timerfd_},
      is_main_loop_{is_main_loop} {
  wakeup_channel_.SetReadCallback([this] { HandleWakeup(); });
  // 将eventfd加入 epoll，等待唤醒
  wakeup_channel_.UseET();
  wakeup_channel_.EnableReading();

  timer_channel_.SetReadCallback([this] { HandleTimer(); });
  timer_channel_.UseET();
  timer_channel_.EnableReading();
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

void EventLoop::HandleTimer() {
  uint64_t expired = 0;
  ::read(timerfd_, &expired, sizeof(expired));
  spdlog::info("component={}_event_loop event=timerfd",
               is_main_loop_ ? "main" : "sub");
  if (!is_main_loop_) {  // main_loop并不持有客户连接connection
    auto now = std::chrono::steady_clock::now();
    // 删掉不活跃连接
    for (auto it = conns_.begin(); it != conns_.end();) {
      auto fd = it->first;
      const auto& conn = it->second;
      if (conn->Timeout(now, 20)) {
        // TODO：lock
        conn_time_out_(fd);     // 通知 TcpServer::RemoveConn(fd)
        it = conns_.erase(it);  // 删 sub loop 的表
      } else {
        ++it;
      }
    }
  }
}

void EventLoop::Run() {
  // spdlog::info("component=event_loop event=run_start");
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

void EventLoop::NewConnection(Connection::Ptr conn) {
  conns_[conn->fd()] = conn;
}
