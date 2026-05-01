#include "poller.h"

#include "channel.h"

namespace net
{
    Poller::Poller(EventLoop* loop) :
        owner_loop_(loop) {}

    Poller::~Poller() = default;

    auto Poller::HasChannel(Channel* channel) const -> bool
    {
        auto it = channels_.find(channel->fd());
        return it != channels_.end() && it->second == channel;
    }
} // namespace net