#pragma once

#include <array>
#include <sys/epoll.h>
#include <vector>

namespace net
{
    class Epoll
    {
    public:
        Epoll();
        ~Epoll();

        void AddFd(int fd, uint32_t op);
        std::vector<epoll_event> Wait(int timeout = -1);
        
    private:
        int fd_{-1};
        std::array<epoll_event, 100> events_{};
    };
}