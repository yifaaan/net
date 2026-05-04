#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace net
{

    class Buffer
    {
    public:
        Buffer();
        ~Buffer();

        void Append(const char* data, size_t size);
        void Clear();
        void Erase(size_t pos, size_t n);
        size_t size() const;
        const char* data() const;

        /** 可读区间。 */
        std::string_view ToStringView() const;

        /**
         * 若当前至少已有 n 字节，返回前 n 字节的只读视图；否则返回默认构造的 string_view（empty）。
         * 用于在凑齐整帧前解析头部中的长度等字段。
         */
        std::string_view PeekPrefix(size_t n) const;

        /** 从缓冲区前端丢弃 n 字节。 */
        void Retrieve(size_t n);

        /**
         * 尝试取出一帧：前 header_size 字节为头部，其后负载长度由 payload_length(header) 给出。
         * 当缓冲区尚未包含 header + 负载时返回 false，且不修改缓冲区。
         * 成功时写入 header、payload，并整体 Retrieve 掉该帧。
         *
         * payload_length 应基于完整头部字节返回负载长度（可为 0）。若返回值过大导致无法与当前缓冲
         * 区对齐，返回 false。
         */
        bool TryRetrieveFrame(
            size_t header_size,
            const std::function<size_t(std::string_view header)>& payload_length,
            std::string& header,
            std::string& payload);

    private:
        std::string buf_;
    };
}
