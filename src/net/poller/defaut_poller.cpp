#include "../poller.h"

namespace net
{
    auto Poller::NewDefaultPoller(EventLoop *loop) -> Poller *
    {
        (void)loop;
        // TODO: return new EpollPoller(loop);
        return nullptr;
    }
}
