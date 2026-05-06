#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"

class TcpServer
{
public:
    TcpServer() : listen_fd_(-1), client_fd_(-1)
    {
    }

    ~TcpServer()
    {
        CloseClient();
        CloseListen();
    }

    bool Init(uint16_t port)
    {
        if ((listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            return false;
        }
        port_ = port;

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = ::htons(port_);
        server_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);

        if (bind(listen_fd_, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        {
            close(listen_fd_);
            listen_fd_ = -1;
            return false;
        }
        if (::listen(listen_fd_, 5) == -1)
        {
            close(listen_fd_);
            listen_fd_ = -1;
            return false;
        }
        return true;
    }

    bool Accept()
    {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        if ((client_fd_ = ::accept(listen_fd_, (sockaddr*)&client_addr, &addr_len)) == -1)
        {
            return false;
        }
        client_ip_ = inet_ntoa(client_addr.sin_addr);
        return true;
    }

    std::string_view client_ip() const
    {
        return client_ip_;
    }

    bool Send(const std::string& buffer)
    {
        if (client_fd_ == -1)
        {
            return false;
        }
        if (::send(client_fd_, buffer.data(), buffer.size(), 0) <= 0)
        {
            return false;
        }
        return true;
    }

    bool Recv(std::string& buffer, const int max_len)
    {
        buffer.clear();
        buffer.resize(max_len);
        int n = ::recv(client_fd_, buffer.data(), buffer.size(), 0);
        if (n <= 0)
        {
            return false;
        }
        buffer.resize(n);
        return true;
    }

    bool Recv(void* buf, const int size)
    {
        int n = ::recv(client_fd_, buf, size, 0);
        if (n <= 0)
        {
            return false;
        }
        return true;
    }

    bool CloseListen()
    {
        if (listen_fd_ == -1)
        {
            return false;
        }

        ::close(listen_fd_);
        listen_fd_ = -1;
        return true;
    }

    bool CloseClient()
    {
        if (client_fd_ == -1)
        {
            return false;
        }

        ::close(client_fd_);
        client_fd_ = -1;
        return true;
    }

    bool RecvFile(const std::string& name, const int size)
    {
        std::ofstream fout;
        fout.open(name, std::ios::binary);
        if (!fout.is_open())
        {
            spdlog::error("打开文件: {} 失败", name);
            return false;
        }
        int readed = 0;
        int to_read = 0;
        char buf[16];

        while (true)
        {
            to_read = size - readed > sizeof(buf) ? sizeof(buf) : size - readed;
            if (!Recv(buf, to_read))
            {
                return false;
            }

            fout.write(buf, to_read);
            readed += to_read;
            if (readed == size)
            {
                break;
            }
        }
        return true;
    }

private:
    int listen_fd_;
    uint16_t port_;

    int client_fd_;
    std::string client_ip_;
};

int main(int argc, char** argv)
{
    net::InitLog();

    TcpServer tcp_server;
    if (!tcp_server.Init(8080))
    {
        spdlog::error("Init server failed: {}", std::strerror(errno));
        return -1;
    }
    if (!tcp_server.Accept())
    {
        spdlog::error("Accept failed: {}", std::strerror(errno));
        return -1;
    }
    spdlog::info("客户端已连接");

    // 接收文件
    // 1.文件名+文件大小
    // 2.回复确认，表示可以发送
    // 3.接收文件内容
    // 4.回复接收成功
    using FileInfo = struct
    {
        char name[20];
        int file_size;
    };
    FileInfo file_info{};
    tcp_server.Recv(&file_info, sizeof(file_info));
    spdlog::info("文件信息: {}, {}-bytes", file_info.name, file_info.file_size);
    tcp_server.Send("ok");

    if (!tcp_server.RecvFile("xxx", file_info.file_size))
    {
        spdlog::error("接收文件内容失败");
        return -1;
    }
    spdlog::info("接收文件内容成功");
    tcp_server.Send("ok");
}
