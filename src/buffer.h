#pragma once

#include <string>
namespace net
{

    class Buffer
    {
    public:
        Buffer();
        ~Buffer();

        void Append(const char* data, size_t size);
        void Clear();
        size_t size() const;
        const char* data() const;

    private:
        std::string buf_;
    };
}