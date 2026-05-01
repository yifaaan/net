#include "epoll_poller.h"

#include <unistd.h>

#include "../logging.h"

namespace
{
    // 还没有在 Poller 中注册的 Channel
    constexpr int kNew = -1;
    // 已经注册到 Poller 中的 Channel
    constexpr int kAdded = 1;
    // 已经从 Poller 中删除的 Channel
    constexpr int kDeleted = 2;
}

namespace net
{
    EpollPoller::EpollPoller(EventLoop* loop) :
        Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
    {
        if (epollfd_ < 0)
        {
            LOG_FATAL << "EpollPoller::EpollPoller";
        }
    }

    EpollPoller::~EpollPoller()
    {
        ::close(epollfd_);
    }

    auto EpollPoller::Poll(int timeout_ms, std::vector<Channel*>& active_channels) -> std::chrono::steady_clock::time_point
    {
        return std::chrono::steady_clock::now();
    }

}