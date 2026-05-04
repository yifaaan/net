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

    std::string_view Buffer::ToStringView() const
    {
        return std::string_view(buf_.data(), buf_.size());
    }

    std::string_view Buffer::PeekPrefix(size_t n) const
    {
        if (buf_.size() < n)
        {
            return {};
        }
        return std::string_view(buf_.data(), n);
    }

    void Buffer::Retrieve(size_t n)
    {
        if (n >= buf_.size())
        {
            buf_.clear();
            return;
        }
        buf_.erase(0, n);
    }
}
