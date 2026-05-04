#include "event_loop.h"

#include "epoll.h"
#include "channel.h"

namespace net
{
    EventLoop::EventLoop() : ep_{ std::make_unique<Epoll>() }
    {
    }
    EventLoop::~EventLoop() = default;

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
}