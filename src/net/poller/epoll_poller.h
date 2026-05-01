#pragma once

#include <sys/epoll.h>

#include "../poller.h"

namespace net
{
    class EpollPoller : public Poller
    {
    public:
        EpollPoller(EventLoop* loop);
        ~EpollPoller() override;

        auto Poll(int timeout_ms, std::vector<Channel*>& active_channels) -> std::chrono::steady_clock::time_point override;
        auto UpdateChannel(Channel* channel) -> void override;
        auto RemoveChannel(Channel* channel) -> void override;

    private:
        static constexpr int kInitEventListSize = 16;

        // 填充就绪事件，将 epoll_wait 返回的就绪事件填入 active_channels。
        auto FillActiveChannels(int num_events, std::vector<Channel*>& active_channels) const -> void;
        // 更新 Channel 在 epoll 中的状态，对应 epoll_ctl 的 ADD / MOD / DEL 操作。
        auto Update(int operation, Channel* channel) -> void;

        int epollfd_;
        // 监听的事件列表
        std::vector<epoll_event> events_;
    };
}