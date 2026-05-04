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
        thread_pool_{ std::make_unique<ThreadPool>(ThreadPoolKind::Io, static_cast<size_t>(num_sub_threads_)) },
        acceptor_{ std::make_unique<Acceptor>(loop_.get(), ip, port) }
    {
        acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::NewConnection, this, std::placeholders::_1));
        loop_->SetTimeoutCallback([this](EventLoop* loop) { OnTimeout(loop); });

        // 每个从循环在独立线程中 Run()。
        for (auto& sub : sub_loops_)
        {
            auto lp = sub.get();
            thread_pool_->AddTask([lp] { lp->Run(); });
        }
    }

    void TcpServer::SetEpollWaitTimeoutMs(int timeout_ms) { loop_->SetWaitTimeoutMs(timeout_ms); }
    void TcpServer::SetIdleTimeoutMs(int timeout_ms) { idle_timeout_ms_ = timeout_ms; }

    void TcpServer::SetMessageHandler(std::function<void(std::shared_ptr<Connection>, std::string, std::string)> cb)
    {
        message_handler_ = std::move(cb);
    }

    void TcpServer::SetSendCompleteHandler(std::function<void(std::shared_ptr<Connection>)> cb)
    {
        send_complete_handler_ = std::move(cb);
    }

    void TcpServer::SetTimeoutHandler(std::function<void(EventLoop*)> cb) { timeout_handler_ = std::move(cb); }

    TcpServer::~TcpServer()
    {
        Stop();
    }

    void TcpServer::Start() { loop_->Run(); }

    EventLoop* TcpServer::PickConnectionLoop()
    {
        if (sub_loops_.empty())
        {
            return loop_.get();
        }
        const size_t idx = next_sub_loop_idx_.fetch_add(1, std::memory_order_relaxed) % sub_loops_.size();
        return sub_loops_[idx].get();
    }

    void TcpServer::Stop()
    {
        bool expected = false;
        if (!stopping_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        {
            return;
        }

        if (acceptor_)
        {
            acceptor_->Stop();
        }

        std::vector<std::shared_ptr<Connection>> all_conns;
        {
            std::lock_guard lock(conns_mutex_);
            all_conns.reserve(conns_.size());
            for (const auto& [fd, conn] : conns_)
            {
                (void)fd;
                all_conns.push_back(conn);
            }
        }
        for (auto& conn : all_conns)
        {
            HandleConnectionExit(std::move(conn), "server shutdown");
        }

        for (auto& sub : sub_loops_)
        {
            if (sub)
            {
                sub->Stop();
            }
        }
        if (thread_pool_)
        {
            thread_pool_->Stop();
        }
        if (loop_)
        {
            loop_->Stop();
        }
    }

    void TcpServer::NewConnection(std::unique_ptr<Socket> client_sock)
    {
        if (stopping_.load(std::memory_order_acquire))
        {
            return;
        }
        auto conn = std::make_shared<Connection>(PickConnectionLoop(), std::move(client_sock));
        conn->SetCloseCallback(std::bind(&TcpServer::CloseConnection, this, std::placeholders::_1));
        conn->SetErrorCallback(std::bind(&TcpServer::ErrorConnection, this, std::placeholders::_1));
        conn->SetMessageCallback(std::bind(
            &TcpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        conn->SetWriteCompleteCallback(std::bind(&TcpServer::OnSendComplete, this, std::placeholders::_1));
        std::cout << std::format("new connection(fd={},ip={},port={}) ok.\n", conn->fd(), conn->ip(), conn->port());

        {
            std::lock_guard lock(conns_mutex_);
            conns_.emplace(conn->fd(), conn);
        }
        TouchConnectionActivity(conn);
    }

    void TcpServer::HandleConnectionExit(std::shared_ptr<Connection> conn, std::string_view reason)
    {
        const int fd = conn->fd();
        std::cout << std::format("client(eventfd={}) {}.\n", fd, reason);
        conn->TearDown();
        std::lock_guard lock(conns_mutex_);
        conns_.erase(fd);
        last_active_at_.erase(fd);
    }

    void TcpServer::TouchConnectionActivity(const std::shared_ptr<Connection>& conn)
    {
        if (!conn)
        {
            return;
        }
        std::lock_guard lock(conns_mutex_);
        last_active_at_[conn->fd()] = std::chrono::steady_clock::now();
    }

    void TcpServer::ClearIdleConnections()
    {
        if (idle_timeout_ms_ <= 0)
        {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto idle_limit = std::chrono::milliseconds(idle_timeout_ms_);
        std::vector<std::shared_ptr<Connection>> expired;
        {
            std::lock_guard lock(conns_mutex_);
            if (conns_.empty())
            {
                return;
            }
            expired.reserve(conns_.size());

            for (const auto& [fd, conn] : conns_)
            {
                const auto it = last_active_at_.find(fd);
                if (it == last_active_at_.end() || (now - it->second) >= idle_limit)
                {
                    expired.push_back(conn);
                }
            }
        }

        for (auto& conn : expired)
        {
            HandleConnectionExit(std::move(conn), "idle timeout");
        }
    }

    void TcpServer::CloseConnection(std::shared_ptr<Connection> conn)
    {
        HandleConnectionExit(std::move(conn), "disconnected");
    }

    void TcpServer::ErrorConnection(std::shared_ptr<Connection> conn)
    {
        HandleConnectionExit(std::move(conn), "error");
    }

    void TcpServer::OnMessage(std::shared_ptr<Connection> conn, std::string header, std::string payload)
    {
        TouchConnectionActivity(conn);
        message_handler_(std::move(conn), std::move(header), std::move(payload));
    }

    void TcpServer::OnSendComplete(std::shared_ptr<Connection> conn)
    {
        TouchConnectionActivity(conn);
        send_complete_handler_(std::move(conn));
    }

    void TcpServer::OnTimeout(EventLoop* loop)
    {
        ClearIdleConnections();
        if (timeout_handler_)
        {
            timeout_handler_(loop);
        }
    }
}