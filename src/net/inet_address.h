#pragma once

#include <netinet/in.h>
#include <variant>
#include <string_view>

namespace net
{
    // Wrapper of sockaddr_in.
    struct InetAddress
    {
    public:
        // 构造 0.0.0.0:port 或 127.0.0.1:port。
        //
        // loopback_only = false: 监听所有网卡，常用于 server bind。
        // loopback_only = true : 只监听本机回环地址。
        explicit InetAddress(uint16_t port = 0, bool loopback_only = false);

        // accept/connect 等系统调用拿到 sockaddr_in 后，可以直接构造 InetAddress。
        explicit InetAddress(const sockaddr_in& addr) noexcept;

        InetAddress(std::string_view ip, uint16_t port);

        std::string ToIp() const;
        std::string ToIpPort() const;
        uint16_t Port() const noexcept;

        // 给 bind/connect 等系统调用使用。
        const sockaddr* GetSockAddr() const noexcept;
        sockaddr* GetMutableSockAddr() noexcept;

        sockaddr_in addr_;
    };
}