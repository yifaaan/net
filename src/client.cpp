// 网络通讯的客户端程序。
#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << std::format("usage:./client ip port\n");
        std::cout << std::format("example:./client 192.168.150.128 5085\n\n");
        return -1;
    }

    const int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
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
        ::close(sockfd);
        return -1;
    }
    if (port_num < 0 || port_num > 65535)
    {
        std::cout << std::format("port out of range.\n");
        ::close(sockfd);
        return -1;
    }
    servaddr.sin_port = htons(static_cast<std::uint16_t>(port_num));

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1)
    {
        std::cout << std::format("invalid address.\n");
        ::close(sockfd);
        return -1;
    }

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) != 0)
    {
        std::cout << std::format("connect({}:{}) failed.\n", argv[1], argv[2]);
        ::close(sockfd);
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
        if (::send(sockfd, buf.data(), len, 0) <= 0)
        {
            std::cout << std::format("write() failed.\n");
            ::close(sockfd);
            return -1;
        }

        buf.fill('\0');
        if (::recv(sockfd, buf.data(), buf.size(), 0) <= 0)
        {
            std::cout << std::format("read() failed.\n");
            ::close(sockfd);
            return -1;
        }

        std::cout << std::format("recv:{}\n", buf.data());
    }

    ::close(sockfd);
    return 0;
}
