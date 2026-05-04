#include "channel.h"

#include <sys/epoll.h>
#include <unistd.h>

#include "epoll.h"
#include "event_loop.h"

namespace net
{
    Channel::Channel(EventLoop* loop, int fd) : loop_{ loop }, fd_{ fd }
    {
    }
    Channel::~Channel() = default;

    int Channel::fd() const
    {
        return fd_;
    }
    void Channel::UseET()
    {
        events_ |= EPOLLET;
    }
    void Channel::EnableReading()
    {
        events_ |= EPOLLIN;
        loop_->UpdateChannel(this);
    }
    void Channel::DisableReading()
    {
        events_ &= ~EPOLLIN;
        loop_->UpdateChannel(this);
    }

    void Channel::EnableWriting()
    {
        events_ |= EPOLLOUT;
        loop_->UpdateChannel(this);
    }
    void Channel::DisableWriting()
    {
        events_ &= ~EPOLLOUT;
        loop_->UpdateChannel(this);
    }
    void Channel::SetInEpoll()
    {
        in_epoll_ = true;
    }
    void Channel::SetNotInEpoll()
    {
        in_epoll_ = false;
    }
    bool Channel::in_epoll() const
    {
        return in_epoll_;
    }

    void Channel::RemoveChannel()
    {
        loop_->RemoveChannel(this);
    }

    void Channel::SetREvents(uint32_t ev)
    {
        revents_ = ev;
    }
    uint32_t Channel::events() const
    {
        return events_;
    }
    uint32_t Channel::revents() const
    {
        return revents_;
    }

    void Channel::HandleEvent()
    {
        if (revents_ & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
        {
            close_callback_();
        } //  普通数据  带外数据
        else if (revents_ & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
        {
            if (on_message_callback_)
                on_message_callback_();
            else
                read_callback_();
        }
        else if (revents_ & EPOLLOUT) // 有数据需要写
        {
            write_callback_();
        }
        else // 其它事件，都视为错误。
        {
            error_callback_();
        }
    }

    void Channel::SetReadCallback(std::function<void()> cb)
    {
        read_callback_ = std::move(cb);
    }

    void Channel::SetOnMessageCallback(std::function<void()> cb)
    {
        on_message_callback_ = std::move(cb);
    }

    void Channel::SetCloseCallback(std::function<void()> cb)
    {
        close_callback_ = std::move(cb);
    }
    void Channel::SetErrorCallback(std::function<void()> cb)
    {
        error_callback_ = std::move(cb);
    }
    void Channel::SetWriteCallback(std::function<void()> cb)
    {
        write_callback_ = std::move(cb);
    }
}