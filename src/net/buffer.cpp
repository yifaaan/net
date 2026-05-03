#include "buffer.h"
#include <sys/uio.h>

namespace net
{
    auto Buffer::ReadFd(int fd, int* saved_errno) -> ssize_t
    {
        char  extra_buf[65536]{}; // 64KB

        // 让一次系统调用可以同时读写多个不连续的内存缓冲区，而无需在用户空间手动拼接/拆分数据
        // - readv(fd, iov, iovcnt) — 从文件描述符读取数据，分散写入多个缓冲区
        // - writev(fd, iov, iovcnt) — 将多个缓冲区的数据聚集后一次性写入
        // - preadv / pwritev — 带偏移量的版本
        iovec vec[2];
        const  size_t writable = WritableBytes();
        vec[0].iov_base = Begin() + writer_index_;
        vec[0].iov_len  = writable;
        vec[1].iov_base  = extra_buf; // buffer写溢出时，写入到栈缓冲区
        vec[1].iov_len = sizeof(extra_buf);

        const int iovcnt = (writable < sizeof(extra_buf)) ? 2 : 1; // writable足够大时，不用栈缓冲
        const  ssize_t n = ::readv(fd, vec, iovcnt);
        if (n < 0)
        {
            *saved_errno = errno;
        }
        else if (n <= writable)
        {
            writer_index_ += n;
        }
        else
        {
            // buffer_写不下
            writer_index_ = buffer_.size(); 
            Append(extra_buf, n - writable);
        }
        return n;
    }
}