#include "echo_server.h"

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
        (void)header;
        if (thread_pool_)
        {
            thread_pool_->AddTask(
                [conn, payload = std::move(payload)]() mutable
                {
                    conn->RunLater(
                        [conn, payload = std::move(payload)]() mutable
                        {
                            conn->Send(payload.data(), payload.size());
                        });
                });
            return;
        }

        conn->Send(payload.data(), payload.size());
    }

    void EchoServer::OnSendComplete(std::shared_ptr<Connection> conn)
    {
        (void)conn;
    }

    void EchoServer::OnTimeout(EventLoop* loop)
    {
        (void)loop;
    }
}
