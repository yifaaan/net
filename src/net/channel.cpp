#include "channel.h"
#include <chrono>

namespace net
{
    Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1)
    {
    }

    Channel::~Channel()
    {
    }

    auto Channel::HandleEvent(std::chrono::steady_clock::time_point receive_time) -> void
    {
        // TODO: 处理事件
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
        // TODO:
    }

} // namespace net