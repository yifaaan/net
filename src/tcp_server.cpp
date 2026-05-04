#include "tcp_server.h"

#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <unistd.h>

#include "acceptor.h"
#include "connection.h"
#include "thread_pool.h"

namespace net
{
    namespace
    {
        /** 创建 n 个独立 EventLoop，供从 reactor 使用（每个对应一个 epoll 实例）。 */
        std::vector<std::unique_ptr<EventLoop>> MakeSubLoops(int n)
        {
            std::vector<std::unique_ptr<EventLoop>> loops;
            loops.reserve(static_cast<size_t>(n));
            for (int i = 0; i < n; ++i)
            {
                loops.push_back(std::make_unique<EventLoop>());
            }
            return loops;
        }
    } // namespace

    TcpServer::TcpServer(const std::string& ip, uint16_t port, int num_sub_threads) :
        loop_{ std::make_unique<EventLoop>() },
        num_sub_threads_{ num_sub_threads > 0 ? num_sub_threads : 1 },
        sub_loops_{ MakeSubLoops(num_sub_threads_) },
        thread_pool_{ std::make_unique<ThreadPool>(static_cast<size_t>(num_sub_threads_)) },
        acceptor_{ std::make_unique<Acceptor>(loop_.get(), ip, port) }
    {
        acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::NewConnection, this, std::placeholders::_1));
        loop_->SetTimeoutCallback([this](EventLoop* loop) { OnTimeout(loop); });

        // 每个从循环在独立线程中 Run()（与池大小一一对应，任务内阻塞不归还 worker）。
        for (auto& sub : sub_loops_)
        {
            EventLoop* const lp = sub.get(); // 按值捕获裸指针，避免引用 sub 的悬垂
            thread_pool_->AddTask([lp] { lp->Run(); });
        }
    }

    void TcpServer::SetEpollWaitTimeoutMs(int timeout_ms) { loop_->SetWaitTimeoutMs(timeout_ms); }

    void TcpServer::SetMessageHandler(std::function<void(Connection*, std::string, std::string)> cb)
    {
        message_handler_ = std::move(cb);
    }

    void TcpServer::SetSendCompleteHandler(std::function<void(Connection*)> cb)
    {
        send_complete_handler_ = std::move(cb);
    }

    void TcpServer::SetTimeoutHandler(std::function<void(EventLoop*)> cb) { timeout_handler_ = std::move(cb); }

    TcpServer::~TcpServer() = default;

    void TcpServer::Start() { loop_->Run(); }

    void TcpServer::NewConnection(std::unique_ptr<Socket> client_sock)
    {
        // 当前仍挂主 loop；若要多 reactor，可改为轮询 sub_loops_[i]->get()。
        auto conn = std::make_unique<Connection>(loop_.get(), std::move(client_sock));
        conn->SetCloseCallback(std::bind(&TcpServer::CloseConnection, this, std::placeholders::_1));
        conn->SetErrorCallback(std::bind(&TcpServer::ErrorConnection, this, std::placeholders::_1));
        conn->SetMessageCallback(std::bind(
            &TcpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        conn->SetWriteCompleteCallback(std::bind(&TcpServer::OnSendComplete, this, std::placeholders::_1));
        std::cout << std::format("new connection(fd={},ip={},port={}) ok.\n", conn->fd(), conn->ip(), conn->port());

        conns_.emplace(conn->fd(), std::move(conn));
    }

    void TcpServer::CloseConnection(Connection* conn)
    {
        std::cout << std::format("client(eventfd={}) disconnected.\n", conn->fd());
        conns_.erase(conn->fd());
    }
    void TcpServer::ErrorConnection(Connection* conn)
    {
        std::cout << std::format("client(eventfd={}) error.\n", conn->fd());
        conns_.erase(conn->fd());
    }

    void TcpServer::OnMessage(Connection* conn, std::string header, std::string payload)
    {
        message_handler_(conn, std::move(header), std::move(payload));
    }

    void TcpServer::OnSendComplete(Connection* conn) { send_complete_handler_(conn); }

    void TcpServer::OnTimeout(EventLoop* loop) { timeout_handler_(loop); }
}