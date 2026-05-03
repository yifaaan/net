#pragma once

#include "inet_address.h"
namespace net
{
    class Socket
    {
    public:
        Socket(int fd);
        ~Socket();
        int fd() const;

        void Bind(const InetAddress& server_addr);
        void Listen(int n = 128);
        int Accept(InetAddress& client_addr);
        void SetReuseAddr(bool on);
        void SetReusePort(bool on);
        void SetTcpNodelay(bool on);
        void SetKeepalive(bool on);

    private:
        const int fd_;

    };

    int CreateNonBlocking();
}