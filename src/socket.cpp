#include "socket.h"

#include <format>
#include <iostream>
#include <netinet/tcp.h>
#include <unistd.h>

#include "inet_address.h"

namespace net
{
    Socket::Socket(int fd) : fd_{ fd }
    {
    }
    Socket::~Socket()
    {
        ::close(fd_);
    }

    int Socket::fd() const
    {
        return fd_;
    }
    void Socket::Bind(const InetAddress& server_addr)
    {
        if (::bind(fd_, server_addr.addr(), sizeof(sockaddr)) < 0)
        {
            ::perror("bind() failed");
            ::close(fd_);
            std::exit(-1);
        }
        SetIpPort(server_addr.ip(), server_addr.port());
    }
    void Socket::Listen(int n)
    {
        if (::listen(fd_, n) != 0)
        {
            ::perror("listen() failed");
            ::close(fd_);
            std::exit(-1);
        }
    }
    int Socket::Accept(InetAddress& client_addr)
    {
        struct sockaddr_in peer_addr;
        socklen_t len = sizeof(peer_addr);
        int clientfd = ::accept4(fd_, (struct sockaddr*)&peer_addr, &len, SOCK_NONBLOCK);

        client_addr.SetAddr(peer_addr);
        // std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n", clientfd, client_addr.ip(), client_addr.port());

        return clientfd;
    }
    void Socket::SetReuseAddr(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof opt)); // 必须的。
    }
    void Socket::SetReusePort(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof opt)); // 有用，但是，在Reactor中意义不大。
    }
    void Socket::SetTcpNodelay(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(fd_, SOL_SOCKET, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof opt)); // 必须的。
    }
    void Socket::SetKeepalive(bool on)
    {
        int opt = on ? 1 : 0;
        ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof opt)); // 可能有用，但是，建议自己做心跳。
    }

    int CreateNonBlocking()
    {
        int listenfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (listenfd < 0)
        {
            ::perror("socket() failed");
            std::exit(-1);
        }
        return listenfd;
    }

    void Socket::SetIpPort(const std::string& ip, uint16_t port)
    {
        ip_ = ip;
        port_ = port;
    }
    const std::string& Socket::ip() const
    {
        return ip_;
    }
    uint16_t Socket::port() const
    {
        return port_;
    }
}