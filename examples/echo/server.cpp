#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

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

private:
    int listen_fd_;
    uint16_t port_;

    int client_fd_;
    std::string client_ip_;
};

int main(int argc, char** argv)
{
    TcpServer tcp_server;
    if (!tcp_server.Init(8080))
    {
        ::perror("Init  server");
        return -1;
    }
    if (!tcp_server.Accept())
    {
        ::perror("Accept");
        return -1;
    }
    std::cout << "客户端已连接\n";

    std::string buffer;
    for (int i = 0; i < 10; i++) // 循环3次，将与服务端进行三次通讯。
    {
        // 接收客户端的报文，如果客户端没有发送回应报文，recv()函数将阻塞等待。
        if (tcp_server.Recv(buffer, 1024) == false)
        {
            perror("recv()");
            break;
        }
        std::cout << "接收：" << buffer << std::endl;

        buffer = "ok";
        if (tcp_server.Send(buffer) == false)
        {
            ::perror("Send");
            break;
        }
        std::cout << "发送: " << buffer << std::endl;
    }
}