#include "event_loop.h"

#include <algorithm>
#include <sys/eventfd.h>
#include <unistd.h>

#include "logging.h"
#include "channel.h"
#include "poller/epoll_poller.h"

namespace
{
    // 每个线程一个 EventLoop，使用 thread_local 判断当前线程是否已经创建过 EventLoop
    thread_local net::EventLoop* KLoopInThisThread = nullptr;

    constexpr auto kPollTimeMs = 10000; // 10 seconds

    // 创建 eventfd，用于 EventLoop::Wakeup() 唤醒 EventLoop 线程。
    auto CreateEventFd() -> int
    {
        // 计数大于 0 时：可读事件，
        // 读完清零后不再可读，直到再次 write
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            LOG_ERROR << "Failed to create eventfd";
            abort();
        }
        return evtfd;
    }
}
namespace net
{
    EventLoop::EventLoop() :
        looping_(false), quit_(false), thread_id_(net::base::CurrentThread::Tid()), poller_(Poller::NewDefaultPoller(this)), wakeup_fd_(CreateEventFd()), wakeup_channel_(new Channel(this, wakeup_fd_))
    {
        LOG_DEBUG << "EventLoop created " << this << " in thread " << thread_id_;
        if (KLoopInThisThread)
        {
            LOG_FATAL << "Another EventLoop " << KLoopInThisThread << " exists in this thread " << thread_id_;
        }
        else
        {
            KLoopInThisThread = this;
        }

        // 设置 wakeup_channel_ 的读事件回调函数为 EventLoop::HandleRead()，
        // 当 wakeup_fd_ 可读时，EventLoop::HandleRead() 会被调用，
        // 从而唤醒 EventLoop 线程执行 pending functors。
        wakeup_channel_->SetReadCallback(std::bind(&EventLoop::HandleRead, this));
        wakeup_channel_->EnableReading();
    }

    EventLoop::~EventLoop()
    {
        LOG_DEBUG << "EventLoop " << this << " of thread " << thread_id_ << " destructs in thread " << net::base::CurrentThread::Tid();
        wakeup_channel_->DisableAll();
        wakeup_channel_->Remove();
        ::close(wakeup_fd_);
        KLoopInThisThread = nullptr;
    }

    auto EventLoop::Loop() -> void
    {
        looping_ = true;
        quit_ = false;
        LOG_TRACE << "EventLoop " << this << " start looping";

        while (!quit_)
        {
            active_channels_.clear();
            auto poll_return_time = poller_->Poll(kPollTimeMs, active_channels_);

            // 处理本轮的所有激活事件
            for (auto channel : active_channels_)
            {
                channel->HandleEvent(poll_return_time);
            }
            // 执行 pendingFunctors_ 中的跨线程/延后任务（可能是别的线程 调用了 RunInLoop(cb)。
            DoPendingFunctors();
        }

        LOG_TRACE << "EventLoop " << this << " stop looping";
        looping_ = false;
    }

    auto EventLoop::Quit() -> void
    {
        quit_ = true;
        // Quit 被非当前 IO 线程调用，需要唤醒
        if (!IsInLoopThread())
        {
            Wakeup();
        }
    }

    auto EventLoop::Wakeup() -> void
    {
        uint64_t one = 1;
        ssize_t n = ::write(wakeup_fd_, &one, sizeof(one));
        if (n != sizeof(one))
        {
            LOG_ERROR << "EventLoop::Wakeup() writes " << n << " bytes instead of 8";
        }
    }

    auto EventLoop::UpdateChannel(Channel* channel) -> void
    {
        poller_->UpdateChannel(channel);
    }

    auto EventLoop::RemoveChannel(Channel* channel) -> void
    {
        poller_->RemoveChannel(channel);
    }

    auto EventLoop::HasChannel(Channel* channel) const -> bool
    {
        return poller_->HasChannel(channel);
    }

    auto EventLoop::HandleRead() -> void
    {
        uint64_t one = 1;
        ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
        if (n != sizeof(one))
        {
            LOG_ERROR << "EventLoop::HandleRead() reads " << n << " bytes instead of 8";
        }
    }

    auto EventLoop::DoPendingFunctors() -> void
    {
        calling_pending_functors_ = true;
        decltype(pending_functors_) tmp;
        {
            std::unique_lock lock(mutex_);
            tmp.swap(pending_functors_);
        }
        for (auto&& f : tmp)
        {
            f();
        }
        calling_pending_functors_ = false;
    }

    auto EventLoop::RunInLoop(std::function<void()> cb) -> void
    {
        if (IsInLoopThread()) // 被当前线程调用
        {
            cb();
        }
        else
        {
            QueueInLoop(std::move(cb));
        }
    }

    auto EventLoop::QueueInLoop(std::function<void()> cb) -> void
    {
        {
            std::unique_lock lock(mutex_);
            pending_functors_.emplace_back(std::move(cb));
        }
        // 被别的 IO 线程调用的，需要 WakeUp，下一轮进行处理。
        // 或者，当前 IO 线程已经将 pending_functors_ swap出来了，正在执行暂存的回调
        // 也需要Wakeup，否则下一轮 Loop时，可能会阻塞到 Poll，不会执行这个 cb
        if (!IsInLoopThread() || calling_pending_functors_)
        {
            Wakeup();
        }
    }
}