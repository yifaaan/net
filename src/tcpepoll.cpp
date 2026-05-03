#include <iostream>
#include <memory>

#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <netinet/tcp.h> // TCP_NODELAY

#include "event_loop.h"
#include "inet_address.h"
#include "socket.h"
#include "epoll.h"
#include "channel.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "usage: ./tcpepoll ip port\n";
        std::cout << "example: ./tcpepoll 192.168.150.128 5085\n\n";
        return -1;
    }

    net::Socket server_sock(net::CreateNonBlocking());
    net::InetAddress server_addr{ argv[1], static_cast<uint16_t>(std::atoi(argv[2])) };
    server_sock.SetReuseAddr(true);
    server_sock.SetReusePort(true);
    server_sock.SetKeepalive(true);
    server_sock.SetTcpNodelay(true);
    server_sock.Bind(server_addr);
    server_sock.Listen();

    net::EventLoop loop;

    auto server_channel = std::make_unique<net::Channel>(loop.ep(), server_sock.fd());
    server_channel->EnableReading();
    server_channel->SetReadCallback(std::bind(&net::Channel::NewConnection, server_channel.get(), server_sock));
    
    loop.Run();

    return 0;
}