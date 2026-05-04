#include "echo_server.h"

#include <format>
#include <iostream>

#include "connection.h"

namespace net
{
    EchoServer::EchoServer(const std::string& ip, uint16_t port, int num_sub_threads, size_t num_worker_threads)
        : server_(ip, port, num_sub_threads),
          num_sub_threads_(num_sub_threads),
          thread_pool_(num_worker_threads > 0 ? std::make_unique<ThreadPool>(ThreadPoolKind::Worker, num_worker_threads)
                                              : nullptr)
    {
        server_.SetMessageHandler([this](std::shared_ptr<Connection> c, std::string h, std::string p) {
            OnMessage(std::move(c), std::move(h), std::move(p));
        });
        server_.SetSendCompleteHandler([this](std::shared_ptr<Connection> c) { OnSendComplete(std::move(c)); });
        server_.SetTimeoutHandler([this](EventLoop* l) { OnTimeout(l); });
    }

    void EchoServer::Start()
    {
        server_.Start();
    }

    void EchoServer::SetEpollWaitTimeoutMs(int timeout_ms)
    {
        server_.SetEpollWaitTimeoutMs(timeout_ms);
    }

    void EchoServer::OnMessage(std::shared_ptr<Connection> conn, std::string header, std::string payload)
    {
        if (thread_pool_)
        {
            thread_pool_->AddTask(
                [conn, header = std::move(header), payload = std::move(payload)]() mutable
                {
                    std::cout << std::format(
                        "recv(eventfd={},ip={},port={}): header={} bytes payload={} bytes [{}]\n",
                        conn->fd(),
                        conn->ip(),
                        conn->port(),
                        header.size(),
                        payload.size(),
                        payload);
                    conn->RunLater(
                        [conn, payload = std::move(payload)]() mutable
                        {
                            conn->Send(payload.data(), payload.size());
                        });
                });
            return;
        }

        std::cout << std::format(
            "recv(eventfd={},ip={},port={}): header={} bytes payload={} bytes [{}]\n",
            conn->fd(),
            conn->ip(),
            conn->port(),
            header.size(),
            payload.size(),
            payload);
        conn->Send(payload.data(), payload.size());
    }

    void EchoServer::OnSendComplete(std::shared_ptr<Connection> conn)
    {
        std::cout << std::format("send complete(eventfd={},ip={},port={})\n", conn->fd(), conn->ip(), conn->port());
    }

    void EchoServer::OnTimeout(EventLoop* loop)
    {
        std::cout << std::format("epoll_wait timeout (loop={})\n", static_cast<void*>(loop));
    }
}
