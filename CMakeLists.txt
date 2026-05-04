cmake_minimum_required(VERSION 3.21)

project(net VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(examples)

add_executable(${PROJECT_NAME}
    src/tcpepoll.cpp
    src/epoll.cpp
    src/inet_address.cpp
    src/socket.cpp
    src/channel.cpp
    src/event_loop.cpp
    src/tcp_server.cpp
    src/acceptor.cpp
    src/connection.cpp
    src/buffer.cpp
)

# 与 examples/echo 的 client 目标区分，避免重名
add_executable(net_client
    src/client.cpp
)

