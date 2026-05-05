cmake_minimum_required(VERSION 3.21)

project(net VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(examples)

add_executable(net_client
    src/client.cc
)

add_executable(net_main
    src/tcpepoll.cc
    src/inet_address.cc
    src/epoll.cc
    src/channel.cc
    src/event_loop.cc
    src/tcp_server.cc
    src/acceptor.cc
    src/connection.cc
    src/buffer.cc
)
