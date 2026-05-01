#pragma once

namespace net::base
{

    // Inherit from Noncopyable when an object owns a resource or represents a
    // unique runtime entity. Examples: file descriptors, sockets, event loops.
    class Noncopyable
    {
    protected:
        Noncopyable() = default;
        ~Noncopyable() = default;

    public:
        Noncopyable(const Noncopyable&) = delete;
        Noncopyable& operator=(const Noncopyable&) = delete;
    };

} // namespace net::base
