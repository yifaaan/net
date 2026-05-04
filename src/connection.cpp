#include "connection.h"

#include "event_loop.h"
#include "socket.h"
#include "channel.h"
#include <array>
#include <cerrno>
#include <cstdint>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
    constexpr size_t kFrameHeaderSize = 4;
    constexpr size_t kMaxPayloadBytes = 16 * 1024 * 1024;

    uint32_t DecodeBe32(std::string_view s)
    {
        const auto* p = reinterpret_cast<const unsigned char*>(s.data());
        return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | uint32_t{p[3]};
    }

    void EncodeBe32(uint32_t v, char out[4])
    {
        auto* p = reinterpret_cast<unsigned char*>(out);
        p[0] = static_cast<unsigned char>((v >> 24) & 0xff);
        p[1] = static_cast<unsigned char>((v >> 16) & 0xff);
        p[2] = static_cast<unsigned char>((v >> 8) & 0xff);
        p[3] = static_cast<unsigned char>(v & 0xff);
    }
} // namespace

namespace net
{
    Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> client_sock) : loop_{ loop }, client_sock_{ std::move(client_sock) }, channel_{ std::make_unique<Channel>(loop_, client_sock_->fd()) }
    {
        // 为新客户端连接准备读事件，并添加到epoll中。
        channel_->UseET();
        channel_->EnableReading();
        channel_->SetOnMessageCallback(std::bind(&Connection::OnMessage, this));
        channel_->SetCloseCallback(std::bind(&Connection::CloseCallback, this));
        channel_->SetErrorCallback(std::bind(&Connection::ErrorCallback, this));
        channel_->SetWriteCallback(std::bind(&Connection::WriteCallback, this));
    }
    Connection::~Connection() = default;

    int Connection::fd() const
    {
        return client_sock_->fd();
    }
    uint16_t Connection::port() const
    {
        return client_sock_->port();
    }
    const std::string& Connection::ip() const
    {
        return client_sock_->ip();
    }

    // 连接断开的回调
    void Connection::CloseCallback()
    {
        close_callback_(this);
    }

    // 错误的回调
    void Connection::ErrorCallback()
    {
        error_callback_(this);
    }

    void Connection::WriteCallback()
    {
        FlushOutput();
    }

    void Connection::FlushOutput()
    {
        if (output_buffer_.size() == 0)
            return;
        while (output_buffer_.size() > 0)
        {
            ssize_t n = ::send(fd(), output_buffer_.data(), output_buffer_.size(), 0);
            if (n > 0)
            {
                output_buffer_.Erase(0, static_cast<size_t>(n));
                continue;
            }
            if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                channel_->EnableWriting();
                return;
            }
            ErrorCallback();
            return;
        }
        channel_->DisableWriting();
        write_complete_callback_(this);
    }

    void Connection::DispatchFrames()
    {
        std::string header;
        std::string payload;
        while (input_buffer_.size() >= kFrameHeaderSize)
        {
            const std::string_view prefix = input_buffer_.PeekPrefix(kFrameHeaderSize);
            const uint32_t len_u = DecodeBe32(prefix);
            if (len_u > kMaxPayloadBytes)
            {
                input_buffer_.Clear();
                ErrorCallback();
                return;
            }
            const size_t plen = static_cast<size_t>(len_u);
            if (!input_buffer_.TryRetrieveFrame(
                    kFrameHeaderSize,
                    [plen](std::string_view) { return plen; },
                    header,
                    payload))
            {
                return;
            }
            message_callback_(this, std::move(header), std::move(payload));
        }
    }

    // 处理对端发来的消息
    void Connection::OnMessage()
    {
        std::array<char, 4096> buffer{};
        while (true)
        {
            ssize_t nread = ::read(fd(), buffer.data(), buffer.size());
            if (nread > 0)
            {
                input_buffer_.Append(buffer.data(), static_cast<size_t>(nread));
                continue;
            }
            if (nread == -1 && errno == EINTR)
            {
                continue;
            }
            if (nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                DispatchFrames();
                break;
            }
            if (nread == 0)
            {
                CloseCallback();
                break;
            }
            ErrorCallback();
            break;
        }
    }

    void Connection::SetCloseCallback(std::function<void(Connection*)> cb)
    {
        close_callback_ = std::move(cb);
    }
    void Connection::SetErrorCallback(std::function<void(Connection*)> cb)
    {
        error_callback_ = std::move(cb);
    }
    void Connection::SetMessageCallback(std::function<void(Connection*, std::string, std::string)> cb)
    {
        message_callback_ = std::move(cb);
    }
    void Connection::SetWriteCompleteCallback(std::function<void(Connection*)> cb)
    {
        write_complete_callback_ = std::move(cb);
    }

    void Connection::Send(const char* data, size_t size)
    {
        if (size > kMaxPayloadBytes)
        {
            ErrorCallback();
            return;
        }
        char hdr[kFrameHeaderSize];
        EncodeBe32(static_cast<uint32_t>(size), hdr);
        output_buffer_.Append(hdr, kFrameHeaderSize);
        output_buffer_.Append(data, size);
        FlushOutput();
    }
}
