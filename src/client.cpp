// 网络通讯的客户端程序（与服务端一致：4 字节大端无符号长度 + 负载）。
#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
    constexpr size_t kMaxPayloadBytes = 16 * 1024 * 1024;

    struct FdCloser
    {
        void operator()(int* fd) const noexcept
        {
            if (fd && *fd >= 0)
            {
                ::close(*fd);
            }
            delete fd;
        }
    };

    void EncodeBe32(uint32_t v, unsigned char* p)
    {
        p[0] = static_cast<unsigned char>((v >> 24) & 0xff);
        p[1] = static_cast<unsigned char>((v >> 16) & 0xff);
        p[2] = static_cast<unsigned char>((v >> 8) & 0xff);
        p[3] = static_cast<unsigned char>(v & 0xff);
    }

    uint32_t DecodeBe32(const unsigned char* p)
    {
        return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | uint32_t{p[3]};
    }

    bool WriteAll(int fd, const void* buf, size_t len)
    {
        const auto* p = static_cast<const char*>(buf);
        size_t off = 0;
        while (off < len)
        {
            const ssize_t n = ::send(fd, p + off, len - off, 0);
            if (n <= 0)
            {
                return false;
            }
            off += static_cast<size_t>(n);
        }
        return true;
    }

    bool ReadAll(int fd, void* buf, size_t len)
    {
        auto* p = static_cast<char*>(buf);
        size_t off = 0;
        while (off < len)
        {
            const ssize_t n = ::recv(fd, p + off, len - off, 0);
            if (n <= 0)
            {
                return false;
            }
            off += static_cast<size_t>(n);
        }
        return true;
    }

    bool SendFrame(int fd, const char* payload, size_t payload_len)
    {
        if (payload_len > kMaxPayloadBytes)
        {
            return false;
        }
        unsigned char hdr[4];
        EncodeBe32(static_cast<uint32_t>(payload_len), hdr);
        if (!WriteAll(fd, hdr, 4))
        {
            return false;
        }
        if (payload_len == 0)
        {
            return true;
        }
        return WriteAll(fd, payload, payload_len);
    }

    bool RecvFrame(int fd, std::string& payload)
    {
        unsigned char hdr[4];
        if (!ReadAll(fd, hdr, 4))
        {
            return false;
        }
        const uint32_t len_u = DecodeBe32(hdr);
        if (len_u > kMaxPayloadBytes)
        {
            return false;
        }
        payload.assign(static_cast<size_t>(len_u), '\0');
        if (len_u == 0)
        {
            return true;
        }
        return ReadAll(fd, payload.data(), static_cast<size_t>(len_u));
    }
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << std::format("usage:./client ip port\n");
        std::cout << std::format("example:./client 192.168.150.128 5085\n\n");
        return -1;
    }

    std::unique_ptr<int, FdCloser> sockfd(new int(::socket(AF_INET, SOCK_STREAM, 0)));
    if (*sockfd < 0)
    {
        std::cout << std::format("socket() failed.\n");
        return -1;
    }

    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;

    int port_num{};
    try
    {
        port_num = std::stoi(argv[2]);
    }
    catch (const std::exception&)
    {
        std::cout << std::format("invalid port.\n");
        return -1;
    }
    if (port_num < 0 || port_num > 65535)
    {
        std::cout << std::format("port out of range.\n");
        return -1;
    }
    servaddr.sin_port = htons(static_cast<std::uint16_t>(port_num));

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1)
    {
        std::cout << std::format("invalid address.\n");
        return -1;
    }

    if (connect(*sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) != 0)
    {
        std::cout << std::format("connect({}:{}) failed.\n", argv[1], argv[2]);
        return -1;
    }

    std::cout << std::format("connect ok.\n");

    std::array<char, 1024> buf{};
    for (int ii = 0; ii < 200000; ++ii)
    {
        buf.fill('\0');
        std::cout << std::format("please input:");
        if (std::scanf("%1023s", buf.data()) != 1)
        {
            break;
        }

        const auto len = std::strlen(buf.data());
        if (!SendFrame(*sockfd, buf.data(), len))
        {
            std::cout << std::format("write() failed.\n");
            return -1;
        }

        std::string reply;
        if (!RecvFrame(*sockfd, reply))
        {
            std::cout << std::format("read() failed.\n");
            return -1;
        }

        std::cout << std::format("recv:{}\n", reply);
    }

    return 0;
}
