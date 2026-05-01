#include "channel.h"
#include <chrono>
#include <sys/poll.h>

namespace net
{
    Channel::Channel(EventLoop* loop, int fd) :
        tied_(false), loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1)
    {
    }

    Channel::~Channel()
    {
        // TODO:
    }

    auto Channel::HandleEvent(std::chrono::steady_clock::time_point receive_time) -> void
    {
        // TODO: 处理事件
        std::shared_ptr<void> guard;
        if (tied_)
        {
            if (guard = tie_.lock(); !guard)
            {
                return;
            }
            handleEventWithGuard(receive_time);
        }
        else
        {
            handleEventWithGuard(receive_time);
        }
    }

    auto Channel::Update() -> void
    {
        // TODO: 更新Channel在Poller中的状态
    }

    auto Channel::Remove() -> void
    {
        // TODO: 从Poller中删除Channel
    }

    auto Channel::handleEventWithGuard(std::chrono::steady_clock::time_point receive_time) -> void
    {
        // 挂起事件, 对方关闭了写端  & 不可读
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            if (close_callback_)
            {
                close_callback_();
            }
        }

        if (revents_ & POLLNVAL)
        {
            // TODO:  fd 无效 记录日志
        }

        if (revents_ & (POLLERR | POLLNVAL))
        {
            if (error_callback_)
            {
                error_callback_();
            }
        }

        if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
        {
            if (read_callback_)
            {
                read_callback_(receive_time);
            }
        }
    }

} // namespace net