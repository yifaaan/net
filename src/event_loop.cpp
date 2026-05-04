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
            auto channels = ep_->Wait();
            for (auto ch : channels)
            {
                ch->HandleEvent();
            }
        }
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