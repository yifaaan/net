#include "epoll_poller.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <vector>

#include "../logging.h"
#include "../channel.h"

namespace
{
    // 还没有在 Poller 中注册的 Channel
    constexpr int kNew = -1;
    // 已经注册到 Poller 中的 Channel
    constexpr int kAdded = 1;
    // 已经从 Poller 中删除的 Channel
    constexpr int kDeleted = 2;

    std::string_view OperationToString(int op)
    {
        switch (op)
        {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            return "Unknown Operation";
        }
    }

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
        LOG_TRACE << "fd total count " << channels_.size();
        int num_events = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeout_ms);
        int err = errno;

        if (num_events > 0)
        {
            FillActiveChannels(num_events, active_channels);
            if (static_cast<size_t>(num_events) == events_.size())
            {
                const auto next_size = events_.size() * static_cast<std::vector<epoll_event>::size_type>(2);
                events_.resize(next_size);
            }
        }
        else if (num_events == 0)
        {
            LOG_TRACE << "nothing happened, timeout = " << timeout_ms << " ms";
        }
        else
        {
            if (err != EINTR)
            {
                errno = err;
                LOG_ERROR << "EpollPoller::Poll()";
            }
        }
        return std::chrono::steady_clock::now();
    }

    // Channle::Update/Remove 会调用 Poller::UpdateChannel/RemoveChannel，最终调用 EPollPoller::UpdateChannel/RemoveChannel。
    auto EpollPoller::UpdateChannel(Channel* channel) -> void
    {
        const int index = channel->index();
        if (index == kNew || index == kDeleted)
        {
            int fd = channel->fd();
            if (index == kNew)
            {
                channels_[fd] = channel;
            }
            channel->set_index(kAdded);
            Update(EPOLL_CTL_ADD, channel);
        }
        else
        {
            if (channel->IsNoneEvent())
            {
                Update(EPOLL_CTL_DEL, channel); // 已注册，但没有事件，从 epoll 中删除 Channel
                channel->set_index(kDeleted);
            }
            else
            {
                Update(EPOLL_CTL_MOD, channel);
            }
        }
    }

    auto EpollPoller::RemoveChannel(Channel* channel) -> void
    {
        int fd = channel->fd();
        int index = channel->index();
        channels_.erase(fd);
        if (index == kAdded)
        {
            Update(EPOLL_CTL_DEL, channel);
        }
        channel->set_index(kNew);
    }

    auto EpollPoller::FillActiveChannels(int num_events, std::vector<Channel*>& active_channels) const -> void
    {
        for (int i = 0; i < num_events; i++)
        {
            auto* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->set_revents(events_[i].events); // 设置 Channel 的 revents_，以便 Channel::HandleEvent() 根据 revents_ 调用相应的回调函数。
            active_channels.push_back(channel);
        }
    }
    
    auto EpollPoller::Update(int operation, Channel* channel) -> void
    {
        epoll_event event{};
        event.events = static_cast<uint32_t>(channel->events());
        int fd = channel->fd();
        event.data.ptr = static_cast<void*>(channel);

        LOG_TRACE << "epoll_ctl op = " << OperationToString(operation) << " fd = " << fd << " event = { " << channel->events() << " }";
        if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
        {
            if (operation == EPOLL_CTL_DEL)
            {
                LOG_ERROR << "epoll_ctl op = " << OperationToString(operation) << " fd = " << fd;
            }
            else
            {
                LOG_FATAL << "epoll_ctl op = " << OperationToString(operation) << " fd = " << fd;
            }
        }
    }

}