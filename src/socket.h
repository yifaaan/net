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

        void SetIpPort(const std::string& ip, uint16_t port);
        const std::string& ip() const;
        uint16_t port() const;
    private:
        const int fd_;
        std::string ip_; // listen fd时 放服务端 ip
        uint16_t port_;
    };

    int CreateNonBlocking();
}