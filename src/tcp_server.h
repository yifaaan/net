#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "event_loop.h"
#include "socket.h"


namespace net
{
    class Acceptor;
    class Connection;
    class ThreadPool;
    class TcpServer
    {
    public:
        TcpServer(const std::string& ip, uint16_t port, int num_sub_threads = 4);
        ~TcpServer();

        // 阻塞运行主 reactor
        void Start();

        // epoll_wait 超时。须 >= 0 才会在每次超时时调用 OnTimeout；-1 表示永久阻塞（默认）。
        void SetEpollWaitTimeoutMs(int timeout_ms);

        void SetMessageHandler(std::function<void(Connection*, std::string, std::string)> cb);
        void SetSendCompleteHandler(std::function<void(Connection*)> cb);
        void SetTimeoutHandler(std::function<void(EventLoop*)> cb);

        void NewConnection(std::unique_ptr<Socket> client_sock);

        void CloseConnection(Connection* conn); // 在Connection中回调
        void ErrorConnection(Connection* conn); // 在Connection中回调

        void OnMessage(Connection* conn, std::string header, std::string payload);
        void OnSendComplete(Connection* conn);
        void OnTimeout(EventLoop* loop);
    private:
        /** 主 reactor：监听与新连接默认都注册在此 loop。 */
        std::unique_ptr<EventLoop> loop_;
        /** 从 reactor 个数，与 sub_loops_ / 线程池 worker 数一致。 */
        int num_sub_threads_;
        /** 从循环：在构造函数里各投递一个 Run() 到线程池，长期占满 worker。 */
        std::vector<std::unique_ptr<EventLoop>> sub_loops_;
        std::unique_ptr<ThreadPool> thread_pool_;
        std::unique_ptr<Acceptor> acceptor_;
        /** fd -> Connection，供 Close/Error 回调按 fd 摘除。 */
        std::unordered_map<int, std::unique_ptr<Connection>> conns_;

        std::function<void(Connection*, std::string, std::string)> message_handler_;
        std::function<void(Connection*)> send_complete_handler_;
        std::function<void(EventLoop*)> timeout_handler_;
    };
}