#include "../poller.h"

namespace net
{
    auto Poller::NewDefaultPoller(EventLoop *loop) -> Poller *
    {
        // TODO: return new EpollPoller(loop);
        return nullptr;
    }
}