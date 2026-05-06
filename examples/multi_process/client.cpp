#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"

class TcpClient
{
public:
    TcpClient() : fd_(-1)
    {
    }

    ~TcpClient()
    {
        Close();
    }

    bool Connect(const std::string& ip, uint16_t port)
    {
        ip_ = ip;
        port_ = port;
        if ((fd_ = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            return false;
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = ::htons(port_);
        if (auto host = ::gethostbyname(ip.c_str()); !host)
        {
            Close();
            return false;
        }
        else
        {
            std::memcpy(&server_addr.sin_addr, host->h_addr_list[0], sizeof(server_addr.sin_addr));
        }
        if (::connect(fd_, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        {
            Close();
            return false;
        }
        return true;
    }

    bool Send(const std::string& buffer)
    {
        if (fd_ == -1)
        {
            return false;
        }
        if (::send(fd_, buffer.data(), buffer.size(), 0) <= 0)
        {
            return false;
        }
        return true;
    }

    bool Recv(std::string& buffer, const int max_len)
    {
        buffer.clear();
        buffer.resize(max_len);
        int n = ::recv(fd_, buffer.data(), buffer.size(), 0);
        if (n <= 0)
        {
            return false;
        }
        buffer.resize(n);
        return true;
    }

    bool Close()
    {
        if (fd_ == -1)
        {
            return false;
        }

        ::close(fd_);
        fd_ = -1;
        return true;
    }

private:
    int fd_;
    std::string ip_;
    uint16_t port_;
};

int main(int argc, char** argv)
{
    net::InitLog();

    if (argc != 3)
    {
        spdlog::error("component=multi_client event=invalid_args usage=\"./client ip port\"");
        spdlog::error("component=multi_client event=usage_example example=\"./client 192.168.101.138 5005\"");
        return -1;
    }

    TcpClient tcp_client;
    if (tcp_client.Connect(argv[1], ::atoi(argv[2])) == false) // 向服务端发起连接请求。
    {
        spdlog::error("component=multi_client event=connect_failed ip={} port={} error={}",
                      argv[1], argv[2], std::strerror(errno));
        return -1;
    }

    // 第3步：与服务端通讯，客户发送一个请求报文后等待服务端的回复，收到回复后，再发下一个请求报文。
    std::string buffer;
    for (int i = 0; i < 10; i++) // 循环3次，将与服务端进行三次通讯。
    {
        buffer = "这是第" + std::to_string(i + 1);
        // 向服务端发送请求报文。
        if (tcp_client.Send(buffer) == false)
        {
            spdlog::error("component=multi_client event=send_failed index={} error={}",
                          i + 1, std::strerror(errno));
            break;
        }
        spdlog::info("component=multi_client event=send_request index={} bytes={} message={}",
                     i + 1, buffer.size(), buffer);

        // 接收服务端的回应报文，如果服务端没有发送回应报文，recv()函数将阻塞等待。
        if (tcp_client.Recv(buffer, 1024) == false)
        {
            spdlog::error("component=multi_client event=recv_failed index={} error={}",
                          i + 1, std::strerror(errno));
            break;
        }
        spdlog::info("component=multi_client event=recv_reply index={} bytes={} message={}",
                     i + 1, buffer.size(), buffer);

        ::sleep(1);
    }
}
