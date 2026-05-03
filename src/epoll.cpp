#include "epoll.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>

namespace net
{
    Epoll::Epoll()
    {
        fd_ = ::epoll_create(1); // 创建epoll句柄（红黑树）。
        if (fd_ <= 0)
        {
            std::exit(-1);
        }
    }

    Epoll::~Epoll()
    {
        ::close(fd_);
    }

    void Epoll::AddFd(int fd, uint32_t op)
    {
        // 为服务端的listenfd准备读事件。
        epoll_event ev;
        ev.data.fd = fd; // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回。
        ev.events = op; // 让epoll监视listenfd的读事件，采用水平触发。
        if (::epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
        {
            std::exit(-1);
        }
    }
    std::vector<epoll_event> Epoll::Wait(int timeout)
    {
        std::vector<epoll_event> evts;
        events_.fill({});
        int infds = ::epoll_wait(fd_, events_.data(), events_.size(), timeout); // 等待监视的fd有事件发生。
        if (infds < 0)
        {
            ::perror("epoll_wait()");
            std::exit(-1);
        }
        if (infds == 0)
        {
            std::cout << "epoll_wait() timeout\n";
            return evts;
        }

        evts.reserve(infds);
        for (int i = 0; i < infds; i++)
        {
            evts.emplace_back(events_[i]);
        }
        return evts;
    }
}