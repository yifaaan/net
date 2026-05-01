#pragma once

#include <chrono>
#include <functional>
#include <string>

#include <sys/epoll.h>

#include "../base/noncopyable.h"

namespace net
{
    class EventLoop;

    // Channel 把“某个 fd 的 IO 事件”封装成对象，供 EventLoop 统一调度。
    // Channel 不拥有 fd，fd 由 Socket/TcpConnection 等上层对象管理。
    // 一个Event Loop 管理多个Channel
    class Channel : public base::Noncopyable
    {
    public:
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(std::chrono::steady_clock::time_point)>;

        Channel(EventLoop* loop, int fd);
        ~Channel();

        // 在 EventLoop 中由 Poller 返回就绪事件后调用。
        auto HandleEvent(std::chrono::steady_clock::time_point receive_time) -> void;

        // 注册回调
        // 可读 -> 读 socket; 可写 -> 发送缓冲区; close/error -> 清理连接
        auto SetReadCallback(ReadEventCallback cb) -> void { read_event_callback_ = std::move(cb); }
        auto SetWriteCallback(EventCallback cb) -> void { write_event_callback_ = std::move(cb); }
        auto SetCloseCallback(EventCallback cb) -> void { close_event_callback_ = std::move(cb); }
        auto SetErrorCallback(EventCallback cb) -> void { error_event_callback_ = std::move(cb); }

        // 绑定TcpConnection
        void tie(const std::shared_ptr<void>& obj)
        {
            tie_ = obj;
            tied_ = true;
        }

        auto fd() const noexcept -> int { return fd_; }

        // 获取感兴趣的事件
        auto events() const noexcept -> int { return events_; }
        // 设置就绪事件, 由Poller设置
        auto set_revents(int revt) -> void { revents_ = revt; }
        // 获取就绪事件
        auto revents() const noexcept -> int { return revents_; }

        // 判断是否为无事件
        auto IsNoneEvent() const noexcept -> bool { return events_ == kNoneEvent; }
        // 判断是否为读事件
        auto IsReading() const noexcept -> bool { return events_ & kReadEvent; }
        // 判断是否为写事件
        auto IsWriting() const noexcept -> bool { return events_ & kWriteEvent; }
        // 启用读事件
        auto EnableReading() -> void
        {
            events_ |= kReadEvent;
            Update();
        }
        // 禁用读事件
        auto DisableReading() -> void
        {
            events_ &= ~kReadEvent;
            Update();
        }
        // 启用写事件
        auto EnableWriting() -> void
        {
            events_ |= kWriteEvent;
            Update();
        }
        // 禁用写事件
        auto DisableWriting() -> void
        {
            events_ &= ~kWriteEvent;
            Update();
        }
        // 禁用所有事件
        auto DisableAll() -> void
        {
            events_ = kNoneEvent;
            Update();
        }

        // 获取在poller中的索引
        auto index() const noexcept -> int { return index_; }
        // 设置在poller中的索引
        auto set_index(int idx) -> void { index_ = idx; }

        auto event_loop() const noexcept -> EventLoop* { return loop_; }

        // 从Poller中删除Channel
        auto Remove() -> void;

    private:
        // 更新Channel在Poller中的状态(epoll_ctl 设置感兴趣的事件)
        auto Update() -> void;

        auto handleEventWithGuard(std::chrono::steady_clock::time_point receive_time) -> void;

        inline static const int kNoneEvent = 0;
        inline static const int kReadEvent = EPOLLIN | EPOLLPRI; // 普通可读（socket 收到数据、监听 socket 有新连接等）| 优先级可读
        inline static const int kWriteEvent = EPOLLOUT; // 可写

        std::weak_ptr<void> tie_; // Channel 本身不拥有 TcpConnection，但它的读写回调会调用 TcpConnection 的成员函数。
        bool tied_; // 通过 tie_ 把 Channel 和 TcpConnection 绑定在一起。
        EventLoop* loop_;
        const int fd_;
        int events_; // 保存感兴趣的事件
        int revents_; // 保存poller返回的具体事件
        int index_; // 在poller中的状态码（kNew/kAdded/kDeleted）

        // 根据revents_ 调用相应的回调函数.

        ReadEventCallback read_event_callback_;
        EventCallback write_event_callback_;
        EventCallback close_event_callback_;
        EventCallback error_event_callback_;
    };
} // namespace net