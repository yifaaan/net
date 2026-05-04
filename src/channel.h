#pragma once

#include "socket.h"

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
        // 表示已加入epoll树上
        void SetInEpoll();
        bool in_epoll() const;

        void SetREvents(uint32_t ev);
        uint32_t events() const;
        uint32_t revents() const;

        // 处理epoll_wait返回的事件
        void HandleEvent();

        // 处理对端发来的消息 
        void OnMessage();

        // 设置读回调
        void SetReadCallback(std::function<void()> read_callback);
    private:
        int fd_{ -1 };
        bool in_epoll_{};
        EventLoop *loop_{};
        uint32_t events_{};
        uint32_t revents_{};
        std::function<void()> read_callback_{};
    };
}