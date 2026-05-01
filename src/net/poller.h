#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>

#include "../base/noncopyable.h"

namespace net
{
    class Channel;
    class EventLoop;

    // IO 多路复用的抽象基类。
    // EventLoop 持有一个 Poller 对象，负责监听多个 Channel 的事件，并在事件发生时通知 EventLoop 调用相应的 Channel 处理事件。
    class Poller : public base::Noncopyable
    {
    public:
        Poller(EventLoop* loop);
        virtual ~Poller();

        // 监听事件，调用 epoll_wait / poll，将就绪事件填入 active_channels。
        // 必须在 loop 线程调用
        virtual auto Poll(int timeout_ms, std::vector<Channel*>& active_channels) -> std::chrono::steady_clock::time_point = 0;

        // 注册或修改 Channel 的事件，对应 epoll_ctl 的 ADD / MOD 操作。
        // 必须在 loop 线程调用
        virtual auto UpdateChannel(Channel* channel) -> void = 0;
        // 从 Poller 中删除 Channel，对应 epoll_ctl 的 DEL 操作。
        // 必须在 loop 线程调用
        virtual auto RemoveChannel(Channel* channel) -> void = 0;

        // 判断 Poller 是否拥有 Channel
        virtual auto HasChannel(Channel* channel) const -> bool;

        static auto NewDefaultPoller(EventLoop* loop) -> Poller*;
    protected:
        // fd -> Channel*
        std::unordered_map<int, Channel*> channels_;

    private:
        EventLoop* owner_loop_;
    };
}
