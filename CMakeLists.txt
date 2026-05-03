cmake_minimum_required(VERSION 3.21)

project(net VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/base/current_thread.cpp
    src/net/timer.cpp
    src/net/inet_address.cpp
    src/net/poller/defaut_poller.cpp
    src/net/channel.cpp
    src/net/poller.cpp
    src/net/poller/epoll_poller.cpp
    src/base/thread.cpp
    src/net/event_loop.cpp
    src/net/event_loop_thread.cpp
    src/net/event_loop_thread_pool.cpp
    src/net/socket.cpp
    src/net/acceptor.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

