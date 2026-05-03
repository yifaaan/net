#include "channel.h"

#include <sys/epoll.h>

#include "epoll.h"

namespace net
{
    Channel::Channel(Epoll* ep, int fd) : ep_{ ep }, fd_{ fd }
    {
    }
    Channel::~Channel() = default;

    int Channel::fd() const
    {
        return fd_;
    }
    void Channel::UseET()
    {
        events_ |= EPOLLET;
    }
    void Channel::EnableReading()
    {
        events_ |= EPOLLIN;
        ep_->UpdateChannel(this);
    }
    void Channel::SetInEpoll()
    {
        in_epoll_ = true;
    }
    bool Channel::in_epoll() const
    {
        return in_epoll_;
    }

    void Channel::SetREvents(uint32_t ev)
    {
        revents_ = ev;
    }
    uint32_t Channel::events() const
    {
        return events_;
    }
    uint32_t Channel::revents() const
    {
        return revents_;
    }
}