#pragma once

#include <netinet/in.h>
#include <string>

namespace net
{
    // socket地址协议
    class InetAddress
    {
    public:
        InetAddress() = default;
        // 服务端使用
        InetAddress(const std::string& ip, uint16_t port);
        // 客户端使用
        InetAddress(sockaddr_in addr);
        ~InetAddress();

        void SetAddr(sockaddr_in addr);

        const char* ip() const;
        uint16_t port() const;
        const sockaddr* addr() const;

    private:
        sockaddr_in addr_;
    };
}