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

        /** epoll 可读时由 Channel 调用：把内核缓冲区数据读入 input_buffer_，读到 EAGAIN 后解析帧。 */
        void OnMessage();

        using MessageCallback = void(Connection*, std::string, std::string);

        void SetCloseCallback(std::function<void(Connection*)> cb);
        void SetErrorCallback(std::function<void(Connection*)> cb);
        void SetMessageCallback(std::function<MessageCallback> cb);
        void SetWriteCompleteCallback(std::function<void(Connection*)> cb);

        /** 发送一帧负载：自动在前面加上 4 字节大端无符号长度（头部），长度为负载字节数。 */
        void Send(const char* data, size_t size);

        /** 将任务投递到本连接所属的 EventLoop 线程（与其它 Channel 回调同线程）。可在工作线程调用。 */
        void RunLater(std::function<void()> fn);

    private:
        void FlushOutput();

        /** 从 input_buffer_ 按「4 字节大端长度 + 负载」拆出完整帧，每帧触发 message_callback_。 */
        void DispatchFrames();

        EventLoop* loop_;
        // 监听socket
        std::unique_ptr<Socket> client_sock_;
        std::unique_ptr<Channel> channel_;

        std::function<void(Connection*)> close_callback_; // 将回调TcpServer::CloseConnection();
        std::function<void(Connection*)> error_callback_;
        std::function<MessageCallback> message_callback_;
        std::function<void(Connection*)> write_complete_callback_;

        Buffer input_buffer_;
        Buffer output_buffer_;
    };
}