#include "channel.h"

#include <format>
#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>

#include "connection.h"
#include "epoll.h"
#include "inet_address.h"
#include "socket.h"
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
    bool Channel::in_epoll() const
    {
        return in_epoll_;
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
            // if (is_listen_fd_) // 如果是listenfd有事件，表示有新的客户端连上来。
            // {
            //     NewConnection(server_sock);
            // }
            // else // 如果是客户端连接的fd有事件。
            // {
            //     OnMessage();
            // }

            read_callback_(); // 使用回调处理读事件
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

    // 处理对端发来的消息
    void Channel::OnMessage()
    {
        std::array<char, 1024> buffer;
        while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
        {
            buffer.fill('0');
            ssize_t nread = ::read(fd_, buffer.data(), buffer.size());
            if (nread > 0) // 成功的读取到了数据。
            {
                // 把接收到的报文内容原封不动的发回去。
                std::cout << std::format("recv(eventfd={}):{}\n", fd_, std::string_view(buffer.data(), static_cast<size_t>(nread)));
                ::send(fd_, buffer.data(), nread, 0);
            }
            else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
            {
                continue;
            }
            else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
            {
                break;
            }
            else if (nread == 0) // 客户端连接已断开。
            {
                std::cout << std::format("client(eventfd={}) disconnected.\n", fd_);
                ::close(fd_); // 关闭客户端的fd。
                break;
            }
        }
    }

    void Channel::SetReadCallback(std::function<void()> cb)
    {
        read_callback_ = std::move(cb);
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