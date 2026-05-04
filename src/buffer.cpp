#include "buffer.h"

namespace net
{
    Buffer::Buffer() = default;
    Buffer::~Buffer() = default;

    void Buffer::Append(const char* data, size_t size)
    {
        buf_.append(data, size);
    }
    void Buffer::Clear()
    {
        buf_.clear();
    }

    void Buffer::Erase(size_t pos, size_t n)
    {
        buf_.erase(pos, n);
    }
    size_t Buffer::size() const
    {
        return buf_.size();
    }
    const char* Buffer::data() const
    {
        return buf_.data();
    }
}