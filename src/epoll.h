#pragma once

#include <array>
#include <sys/epoll.h>
#include <vector>

namespace net
{
    class Channel;

    class Epoll
    {
    public:
        Epoll();
        ~Epoll();

        // void AddFd(int fd, uint32_t op);
        void UpdateChannel(Channel* channel);
        // std::vector<epoll_event> Wait(int timeout = -1);
        std::vector<Channel*> Wait(int timeout = -1);
        
    private:
        int fd_{-1};
        std::array<epoll_event, 100> events_{};
    };
}