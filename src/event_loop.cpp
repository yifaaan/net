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
        while (true)
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

    void EventLoop::SetWaitTimeoutMs(int timeout_ms)
    {
        wait_timeout_ms_ = timeout_ms;
    }

    void EventLoop::SetTimeoutCallback(std::function<void(EventLoop*)> cb)
    {
        timeout_callback_ = std::move(cb);
    }

    Epoll* EventLoop::ep() const
    {
        return ep_.get();
    }
    void EventLoop::UpdateChannel(Channel *ch)
    {
        ep_->UpdateChannel(ch);
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
        constexpr uint64_t one = 1;
        const ssize_t nw = ::write(wakeup_fd_, &one, sizeof one);
        if (nw != static_cast<ssize_t>(sizeof one))
        {
            ::perror("eventfd write");
            std::exit(-1);
        }
    }
}