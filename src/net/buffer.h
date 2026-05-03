#pragma once

#include <string>
#include <vector>

namespace net
{
    //
    // +---------------------+--------------------+--------------------+
    // |   prependable 区    |    readable 区    |    writable 区     |
    // |  (为包头预留空间)    |     (已读入数据)   |    (剩余可写空间)    |
    // +---------------------+--------------------+--------------------+
    // |                     |                    |                    |
    // 0           <=    reader_index_    <=   writer_index_    <=   size()
    //
    class Buffer
    {
    public:
        // 预留给协议头的空间
        static constexpr size_t kCheapPrepend = 8;
        // 初始 buffer 大小 1KB
        static constexpr size_t kInitialSize = 1024;

        explicit Buffer(size_t initial_size = kInitialSize) : buffer_(kCheapPrepend + initial_size), reader_index_(kCheapPrepend), writer_index_(kCheapPrepend)
        {
        }

        // 可读数据长度
        auto ReadableBytes() const noexcept -> size_t { return writer_index_ - reader_index_; }
        // 可写缓冲区数据长度
        auto WritableBytes() const noexcept -> size_t { return buffer_.size() - writer_index_; }
        // 已被读消费的字节数
        auto PrependableBytes() const noexcept -> size_t { return reader_index_; }

        auto Peek() const noexcept -> const char* { return Begin() + reader_index_; }
        auto Peek() noexcept -> char* { return Begin() + reader_index_; }

        // 读取 len 长度，设置对应的下标
        auto Retrieve(size_t len) -> void
        {
            if (len < ReadableBytes())
            {
                reader_index_ += len;
            }
            else
            {
                RetrieveAll();
            }
        }

        auto RetrieveAll() -> void
        {
            reader_index_ = writer_index_ = kCheapPrepend;
        }

        // 读取len字节数据
        auto RetrieveAsString(size_t len) -> std::string
        {
            std::string result(Peek(), len);
            Retrieve(len);
            return result;
        }

        // 读所有可读数据
        auto RetrieveAllAsString() -> std::string
        {
            return RetrieveAsString(ReadableBytes());
        }

        auto Append(const char* data, size_t len) -> void
        {
            EnsureWritableBytes(len);
            std::copy(data, data + len, BeginWrite());
            HasWritten(len);
        }

        auto Append(const void* data, size_t len) -> void
        {
            Append(static_cast<const char*>(data), len);
        }

        auto Append(const std::string& str) -> void
        {
            Append(str.data(), str.size());
        }

        // 在数据前插入head，len必须小于 PrependableBytes
        auto Prepend(const void* data, size_t len) -> void
        {
            reader_index_ -= len;
            const auto* d = static_cast<const char*>(data);
            std::copy(d, d + len, Begin() + reader_index_);
        }

        auto ReadFd(int fd, int* saved_errno) -> ssize_t;

    private:
        auto Begin() const noexcept -> const char* { return buffer_.data(); }
        auto Begin() noexcept -> char* { return buffer_.data(); }
        auto BeginWrite() noexcept -> char* { return Begin() + writer_index_; }
        auto HasWritten(size_t len) -> void { writer_index_ += len; }

        // 确保可以写入len长度的 数据
        auto EnsureWritableBytes(size_t len) -> void
        {
            if (WritableBytes() < len)
            {
                MakeSpace(len);
            }
        }

        //                     readerIndex_        writerIndex_
        //                     ↓                    ↓
        // +-------------------+--------------------+------------------+
        // | prependable bytes |  readable bytes    |  writable bytes  |
        // |   (已被消费)       |   (还不能丢)      |   (空闲)           |
        // +-------------------+--------------------+------------------+
        // 0            prependable    := readerIndex_
        // readable       := writerIndex_ - readerIndex_
        // writable       := buffer_.size() - writerIndex_
        // 扩容
        auto MakeSpace(size_t len) -> void
        {
            if (WritableBytes() + PrependableBytes() < len + kCheapPrepend) // 在不分配新内存的前提下、通过碎片整理能动员的最大可用空间
            {
                buffer_.resize(writer_index_ + len);
            }
            else
            {
                // 将所有可读数据移到开头部分
                auto readable = ReadableBytes();
                std::copy(Begin() + reader_index_, Begin() + writer_index_, Begin() + kCheapPrepend);
                reader_index_ = kCheapPrepend;
                writer_index_ = reader_index_ + readable;
            }
        }
        std::vector<char> buffer_;
        size_t reader_index_;
        size_t writer_index_;
    };
}