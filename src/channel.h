#pragma once

#include <cstdint>
#include <functional>

namespace net
{
    class EventLoop;

    class Channel
    {
    public:
        Channel(EventLoop* loop, int fd);
        ~Channel();

        int fd() const;
        // 设置边缘触发
        void UseET();
        // 让epoll_wait监听读事件
        void EnableReading();
        void DisableReading();

        void EnableWriting();
        void DisableWriting();
        // 表示已加入epoll树上
        void SetInEpoll();
        void SetNotInEpoll();
        bool in_epoll() const;

        /** 从本 loop 的 epoll 实例摘除（EPOLL_CTL_DEL）；未加入则为空操作。 */
        void RemoveChannel();

        void SetREvents(uint32_t ev);
        uint32_t events() const;
        uint32_t revents() const;

        // 处理epoll_wait返回的事件
        void HandleEvent();

        // 监听 fd：SetReadCallback（如新连接）。业务 fd：SetOnMessageCallback（可读数据）。
        void SetReadCallback(std::function<void()> cb);
        void SetOnMessageCallback(std::function<void()> cb);

        void SetCloseCallback(std::function<void()> cb);
        void SetErrorCallback(std::function<void()> cb);
        void SetWriteCallback(std::function<void()> cb);
    private:
        int fd_{ -1 };
        bool in_epoll_{};
        EventLoop *loop_{};
        uint32_t events_{};
        uint32_t revents_{};
        std::function<void()> read_callback_{}; // listen fd 读事件，如 Acceptor::NewConnection
        std::function<void()> on_message_callback_{}; // 已连接 fd 可读：Connection::OnMessage
        std::function<void()> close_callback_{}; // Connection::CloseCallback();
        std::function<void()> error_callback_{}; // Connection::ErrorCallback();
        std::function<void()> write_callback_{};
    };
}