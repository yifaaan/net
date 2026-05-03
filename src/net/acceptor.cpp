#include "acceptor.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "event_loop.h"
#include "inet_address.h"
#include "logging.h"

namespace net
{
    // accept_socket_也要设置成 NONBLOCK
    // 当 epoll 告诉我们"fd 可读"时，它只保证在 epoll_wait 返回的那个时刻，fd 是可读的。 它不保证当我们真正调用 accept() 时，连接还在 backlog 里
    // 若 另一个进程/线程也监听了同一端口 (SO_REUSEPORT)，它抢先 accept 了这个连接
    // 当前线程调用 accept() → 阻塞
    Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port)
        : loop_(loop)
        , accept_socket_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0))
        , accept_channel_(loop, accept_socket_.fd())
        , listening_(false)
        , idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
    {
        if (accept_socket_.fd() < 0)
        {
            LOG_FATAL << "Acceptor::Acceptor - socket() failed";
        }
        if (idle_fd_ < 0)
        {
            LOG_FATAL << "Acceptor::Acceptor - open /dev/null failed";
        }

        accept_socket_.SetReuseAddr(true);
        accept_socket_.SetReusePort(reuse_port);
        accept_socket_.BindAddress(listen_addr);

        accept_channel_.SetReadCallback(std::bind(&Acceptor::HandleRead, this));
    }

    Acceptor::~Acceptor()
    {
        accept_channel_.DisableAll();
        accept_channel_.Remove();
        ::close(idle_fd_);
    }

    auto Acceptor::Listen() -> void
    {
        listening_ = true;
        accept_socket_.Listen();
        accept_channel_.EnableReading();
    }

    auto Acceptor::HandleRead() -> void
    {
        InetAddress peer_addr;
        int connfd = accept_socket_.Accept(&peer_addr);

        if (connfd >= 0)
        {
            if (new_connection_callback_)
            {
                new_connection_callback_(connfd, peer_addr);
            }
            else
            {
                ::close(connfd);
            }
        }
        else
        {
            // EMFILE: 进程打开的文件描述符达到上限。
            // 此时监听 socket 仍然可读（连接还在 backlog 里），
            // 如果不处理，epoll 会不断触发可读事件，造成 busy loop。
            //
            // 处理策略：
            // 1. 关闭预留的 idle_fd_，空出一个 fd 位置
            // 2. accept 这个积压的连接
            // 3. 立即关闭它（抛弃该连接）
            // 4. 重新打开 /dev/null，恢复预留的空位
            if (errno == EMFILE)
            {
                LOG_ERROR << "Acceptor::HandleRead - EMFILE, too many open files";
                ::close(idle_fd_);
                idle_fd_ = ::accept(accept_socket_.fd(), nullptr, nullptr);
                ::close(idle_fd_);
                idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            }
            else
            {
                LOG_ERROR << "Acceptor::HandleRead - accept() failed, errno=" << errno;
            }
        }
    }
} // namespace net
