#include <iostream>
#include <thread>

#include "echo_server.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "usage: ./tcpepoll ip port\n";
        std::cout << "example: ./tcpepoll 192.168.150.128 5085\n\n";
        return -1;
    }

    const unsigned int hc = std::thread::hardware_concurrency();
    const int sub_threads = hc > 1 ? static_cast<int>(hc - 1) : 1;
    net::EchoServer server{ argv[1], static_cast<uint16_t>(std::atoi(argv[2])), sub_threads };

    server.Start();

    return 0;
}