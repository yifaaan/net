#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

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

    bool Send(const void* buf, const int size)
    {
        if (fd_ == -1)
        {
            return false;
        }
        if (::send(fd_, buf, size, 0) <= 0)
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

    bool SendFile(const std::string& name, const int size)
    {
        std::ifstream fin(name, std::ios::binary);
        if (!fin.is_open())
        {
            std::cout << "打开文件: " << name << " 失败\n";
            return false;
        }
        int to_read = 0;
        int readed = 0;
        char buf[16];

        while (true)
        {
            std::memset(buf, 0, sizeof(buf));
            to_read = size - readed > sizeof(buf) ? sizeof(buf) : size - readed;
            fin.read(buf, to_read);
            if (!Send(buf, to_read))
            {
                return false;
            }
            readed += to_read;
            if (readed == size)
            {
                break;
            }
        }
        return true;
    }

private:
    int fd_;
    std::string ip_;
    uint16_t port_;
};

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cout << "Using:./client 服务端的IP 服务端的端口 文件名 大小(Byte)\nExample:./client 192.168.101.138 5005 a.txt 20\n\n";
        return -1;
    }

    TcpClient tcp_client;
    if (tcp_client.Connect(argv[1], ::atoi(argv[2])) == false) // 向服务端发起连接请求。
    {
        perror("connect()");
        return -1;
    }

    // 发送文件
    // 1.文件名+文件大小
    // 2.等待确认
    // 3.发送文件内容
    // 4.等待确认

    using FileInfo = struct
    {
        char name[20];
        int file_size;
    };
    FileInfo file_info{ .file_size = ::atoi(argv[4]) };
    ::strcpy(file_info.name, argv[3]);
    tcp_client.Send(&file_info, sizeof(file_info));
    std::cout << "文件信息发送成功" << std::endl;

    std::string buffer;
    if (!tcp_client.Recv(buffer, 2))
    {
        ::perror("Recv");
        return -1;
    }
    if (buffer != "ok")
    {
        std::cout << "服务端没有确认回复\n";
        return -1;
    }
    tcp_client.SendFile(file_info.name, file_info.file_size);

    if (!tcp_client.Recv(buffer, 2))
    {
        ::perror("Recv");
        return -1;
    }
    if (buffer != "ok")
    {
        std::cout << "服务端没有确认回复\n";
        return -1;
    }
    std::cout << "文件发送成功\n";

}