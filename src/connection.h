#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "buffer.h"

namespace net
{

    class EventLoop;
    class Socket;
    class Channel;

    // 封装处理用户请求的channel
    class Connection
    {
    public:
        Connection(EventLoop* loop, std::unique_ptr<Socket> client_sock);
        ~Connection();

        int fd() const;
        uint16_t port() const;
        const std::string& ip() const;

        // 连接断开的回调
        void CloseCallback();

        // 错误的回调 
        void ErrorCallback();

        void WriteCallback();
        void OnMessage();

        void SetCloseCallback(std::function<void(Connection*)> cb);
        void SetErrorCallback(std::function<void(Connection*)> cb);
        void SetMessageCallback(std::function<void(Connection*, std::string)> cb);
        void SetWriteCompleteCallback(std::function<void(Connection*)> cb);

        void Send(const char* data, size_t size);

    private:
        void FlushOutput();

        EventLoop* loop_;
        // 监听socket
        std::unique_ptr<Socket> client_sock_;
        std::unique_ptr<Channel> channel_;

        std::function<void(Connection*)> close_callback_; // 将回调TcpServer::CloseConnection();
        std::function<void(Connection*)> error_callback_;
        std::function<void(Connection*, std::string)> message_callback_;
        std::function<void(Connection*)> write_complete_callback_;

        Buffer input_buffer_;
        Buffer output_buffer_;
    };
}