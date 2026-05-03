#pragma once

#include <functional>

#include "../base/noncopyable.h"
#include "channel.h"
#include "socket.h"

namespace net
{
    class EventLoop;
    struct InetAddress;

    // Acceptor 监听端口，接受（accept）新的 TCP 连接。
    // 由 TcpServer 持有，属于服务端内部组件。
    class Acceptor : public base::Noncopyable
    {
    public:
        using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& peer_addr)>;

        Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port = false);
        ~Acceptor();

        auto SetNewConnectionCallback(const NewConnectionCallback& cb) -> void
        {
            new_connection_callback_ = cb;
        }

        auto Listen() -> void;

        auto IsListening() const noexcept -> bool { return listening_; }

    private:
        // 读事件的回调：接受新的新连接
        auto HandleRead() -> void;

        EventLoop* loop_;
        Socket accept_socket_;
        Channel accept_channel_;
        NewConnectionCallback new_connection_callback_;
        bool listening_;
        int idle_fd_; // EMFILE 应急预留 fd
    };
} // namespace net
