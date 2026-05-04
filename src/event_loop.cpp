#include "event_loop.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <sys/eventfd.h>
#include <unistd.h>

#include "channel.h"
#include "epoll.h"

namespace net
{
    EventLoop::EventLoop() : ep_{ std::make_unique<Epoll>() }
    {
        wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (wakeup_fd_ < 0)
        {
            ::perror("eventfd()");
            std::exit(-1);
        }
        wakeup_channel_ = std::make_unique<Channel>(this, wakeup_fd_);
        wakeup_channel_->SetReadCallback([this] { WakeupRead(); });
        wakeup_channel_->EnableReading();
    }

    EventLoop::~EventLoop()
    {
        if (wakeup_channel_)
        {
            wakeup_channel_->RemoveChannel();
        }
        wakeup_channel_.reset();
        if (wakeup_fd_ >= 0)
        {
            ::close(wakeup_fd_);
            wakeup_fd_ = -1;
        }
    }

    // 运行事件循环
    void EventLoop::Run()
    {
        quit_.store(false, std::memory_order_release);
        while (!quit_.load(std::memory_order_acquire))
        {
            auto channels = ep_->Wait(wait_timeout_ms_);
            if (channels.empty())
            {
                if (wait_timeout_ms_ >= 0 && timeout_callback_)
                    timeout_callback_(this);
                continue;
            }
            for (auto ch : channels)
            {
                ch->HandleEvent();
            }
        }
    }

    void EventLoop::Stop()
    {
        quit_.store(true, std::memory_order_release);
        WakeupWrite();
    }

    void EventLoop::SetWaitTimeoutMs(int timeout_ms)
    {
        wait_timeout_ms_ = timeout_ms;
    }

    void EventLoop::SetTimeoutCallback(std::function<void(EventLoop*)> cb)
    {
        timeout_callback_ = std::move(cb);
    }

    void EventLoop::Wakeup()
    {
        WakeupWrite();
    }

    Epoll* EventLoop::ep() const
    {
        return ep_.get();
    }
    void EventLoop::UpdateChannel(Channel *ch)
    {
        ep_->UpdateChannel(ch);
    }

    void EventLoop::RemoveChannel(Channel* ch)
    {
        ep_->RemoveChannel(ch);
    }

    void EventLoop::WakeupRead()
    {
        uint64_t u{};
        while (true)
        {
            const ssize_t n = ::read(wakeup_fd_, &u, sizeof u);
            if (n == static_cast<ssize_t>(sizeof u))
            {
                continue;
            }
            if (n == -1 && errno == EINTR)
            {
                continue;
            }
            if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                break;
            }
            ::perror("eventfd read");
            std::exit(-1);
        }

        std::deque<std::function<void()>> tasks;
        {
            std::lock_guard lock(pending_mutex_);
            tasks.swap(pending_functors_);
        }
        for (auto& f : tasks)
        {
            if (f)
            {
                f();
            }
        }
    }

    void EventLoop::RunLater(std::function<void()> fn)
    {
        {
            std::lock_guard lock(pending_mutex_);
            pending_functors_.push_back(std::move(fn));
        }
        WakeupWrite();
    }

    void EventLoop::WakeupWrite()
    {
        constexpr uint64_t one = 1;
        while (true)
        {
            const ssize_t nw = ::write(wakeup_fd_, &one, sizeof one);
            if (nw == static_cast<ssize_t>(sizeof one))
            {
                return;
            }
            if (nw == -1 && errno == EINTR)
            {
                continue;
            }
            if (nw == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // eventfd 计数已非零，loop 会被唤醒；此时无需再次写入。
                return;
            }
            ::perror("eventfd write");
            std::exit(-1);
        }
    }
}