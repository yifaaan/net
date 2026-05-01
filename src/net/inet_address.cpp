#include "inet_address.h"

#include <arpa/inet.h>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace net
{
    InetAddress::InetAddress(uint16_t port, bool loopback_only)
    {
        addr_.sin_family = AF_INET;

        // INADDR_ANY      表示 0.0.0.0，server bind 后监听所有网卡。
        // INADDR_LOOPBACK 表示 127.0.0.1，只允许本机访问。
        const in_addr_t ip = loopback_only ? INADDR_LOOPBACK : INADDR_ANY;

        // sockaddr_in 里保存的 IP 和端口必须是网络字节序。
        addr_.sin_addr.s_addr = ::htonl(ip);
        addr_.sin_port = ::htons(port);
    }

    InetAddress::InetAddress(std::string_view ip, uint16_t port)
    {
        addr_.sin_family = AF_INET;
        addr_.sin_port = ::htons(port);

        // inet_pton 把 "127.0.0.1" 这样的字符串转成网络字节序的二进制 IP。
        std::string ip_string(ip);
        if (::inet_pton(AF_INET, ip_string.c_str(), &addr_.sin_addr) <= 0)
        {
            throw std::invalid_argument("invalid IPv4 address: " + ip_string);
        }
    }

    InetAddress::InetAddress(const sockaddr_in& addr) noexcept : addr_(addr)
    {
    }

    std::string InetAddress::ToIp() const
    {
        char buf[INET_ADDRSTRLEN] = {};

        // inet_ntop 把二进制 IP 转成 "127.0.0.1" 这样的字符串。
        const char* result = ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
        return result == nullptr ? std::string{} : std::string(buf);
    }

    std::string InetAddress::ToIpPort() const
    {
        char buf[INET_ADDRSTRLEN + 16] = {};
        std::snprintf(buf, sizeof(buf), "%s:%u", ToIp().c_str(), Port());
        return buf;
    }

    uint16_t InetAddress::Port() const noexcept
    {
        return ::ntohs(addr_.sin_port);
    }

    const sockaddr* InetAddress::GetSockAddr() const noexcept
    {
        return reinterpret_cast<const sockaddr*>(&addr_);
    }

    sockaddr* InetAddress::GetMutableSockAddr() noexcept
    {
        return reinterpret_cast<sockaddr*>(&addr_);
    }
}
