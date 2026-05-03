#pragma once

#include <cstdint>

namespace net
{
    class Epoll;

    class Channel
    {
    public:
        Channel(Epoll* ep, int fd);
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

    private:
        int fd_{ -1 };
        bool in_epoll_{};
        Epoll* ep_{};
        uint32_t events_{};
        uint32_t revents_{};
    };
}