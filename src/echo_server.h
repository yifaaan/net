#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "tcp_server.h"
#include "thread_pool.h"

namespace net
{
    class Connection;
    class EventLoop;

    class EchoServer
    {
    public:
        /** @param num_worker_threads 业务工作线程数；为 0 时消息在 I/O 线程同步处理（与原先行为一致）。 */
        EchoServer(const std::string& ip, uint16_t port, int num_sub_threads = 0, size_t num_worker_threads = 0);

        void Start();
        void SetEpollWaitTimeoutMs(int timeout_ms);

    private:
        void OnMessage(Connection* conn, std::string header, std::string payload);
        void OnSendComplete(Connection* conn);
        void OnTimeout(EventLoop* loop);

        TcpServer server_;
        int num_sub_threads_;
        std::unique_ptr<ThreadPool> thread_pool_;
    };
}
