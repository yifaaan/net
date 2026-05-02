#include "socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "inet_address.h"
#include "logging.h"

namespace
{
    auto CloseFd(int fd) noexcept -> void
    {
        if (fd >= 0)
        {
            ::close(fd);
        }
    }

    [[noreturn]] auto SyscallAbort(const char* what) -> void
    {
        const int err = errno;
        LOG_FATAL << what << " failed, errno=" << err << " (" << std::strerror(err) << ")";
        std::abort();
    }
}

namespace net
{
    Socket::Socket(int sockfd) noexcept : sockfd_(sockfd)
    {
    }

    Socket::~Socket()
    {
        CloseFd(sockfd_);
    }

    auto Socket::BindAddress(const InetAddress& local_addr) -> void
    {
        if (::bind(sockfd_, local_addr.GetSockAddr(), sizeof(sockaddr_in)) < 0)
        {
            SyscallAbort("bind");
        }
    }

    auto Socket::Listen() -> void
    {
        if (::listen(sockfd_, SOMAXCONN) < 0)
        {
            SyscallAbort("listen");
        }
    }

    auto Socket::Accept(InetAddress* peer_addr) -> int
    {
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);

        // 设置 nonblocking + cloexec
        int connfd = ::accept4(sockfd_,
                               reinterpret_cast<sockaddr*>(&addr),
                               &len,
                               SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (connfd >= 0 && peer_addr)
        {
            *peer_addr = InetAddress(addr);
        }

        return connfd;
    }

    auto Socket::ShutdownWrite() -> void
    {
        if (::shutdown(sockfd_, SHUT_WR) < 0)
        {
            const int err = errno;
            LOG_ERROR << "shutdown(SHUT_WR) failed, errno=" << err << " (" << std::strerror(err) << ")";
        }
    }

    auto Socket::SetTcpNoDelay(bool on) -> void
    {
        const int optval = on ? 1 : 0;
        ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
    }

    auto Socket::SetReuseAddr(bool on) -> void
    {
        const int optval = on ? 1 : 0;
        ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
    }

    auto Socket::SetReusePort(bool on) -> void
    {
#ifdef SO_REUSEPORT
        const int optval = on ? 1 : 0;
        const int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
        if (ret < 0 && on)
        {
            const int err = errno;
            LOG_ERROR << "setsockopt(SO_REUSEPORT) failed, errno=" << err << " (" << std::strerror(err) << ")";
        }
#else
        if (on)
        {
            LOG_WARN << "SO_REUSEPORT is not supported on this platform.";
        }
#endif
    }

    auto Socket::SetKeepAlive(bool on) -> void
    {
        const int optval = on ? 1 : 0;
        ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
    }
} // namespace net
