#pragma once

#include "../base/noncopyable.h"

struct tcp_info;

namespace net
{
    struct InetAddress;

    // Socket fd 的 RAII 封装。
    class Socket : public base::Noncopyable
    {
    public:
        explicit Socket(int sockfd) noexcept;
        ~Socket();

        auto fd() const noexcept -> int { return sockfd_; }

        // abort if address in use / syscall fails
        auto BindAddress(const InetAddress& local_addr) -> void;
        auto Listen() -> void;

        // 返回已设置为 nonblocking + cloexec 的 connfd；失败返回 -1
        auto Accept(InetAddress* peer_addr) -> int;

        auto ShutdownWrite() -> void;

        // 禁用 Nagle 算法，效果：数据立即发送，不等待合并
        // 否则：write后，数据积压在内核缓冲区等待 ACK，延迟一段时间才发送
        auto SetTcpNoDelay(bool on) -> void;

        // SO_REUSEADDR解决的问题 — TIME_WAIT 占端口，设置之后服务端可以快速重启
        // 重启服务器时 bind() 失败，报 EADDRINUSE（Address already in use）。
        // 主动关闭方（服务器）                    被动关闭方（客户端）
        // ─────────────────                    ─────────────────
        //        │                                    │
        //        │──────── FIN ──────────────────────→│
        //        │                                    │
        //        │←──────── ACK ──────────────────────│
        //        │                                    │
        //        │←──────── FIN ──────────────────────│
        //        │                                    │
        //        │──────── ACK ──────────────────────→│
        //        │                                    │
        //        │◄── TIME_WAIT (2MSL ≈ 60-120s) ────│
        //        │    端口被锁定，无法 bind()          │
      
        // 当服务器主动关闭连接（最后发送 ACK 的一端）进入 TIME_WAIT 状态，持续 2MSL（Maximum Segment
        //  Lifetime，Linux 默认 60 秒）。在这段时间内，该端口上的 四元组（src_ip, src_port, dst_ip, dst_port）
        // 被内核锁定，任何企图 bind() 同一端口的操作都会返回 EADDRINUSE。

        // 等 2MSL 为了防止残留的延迟报文被误认为属于新连接
        // 客户端 10.0.0.1:54321  ←→  服务器 10.0.0.2:8080
        // 序列号范围：seq=1000~5000

        // 场景还原
        // 步骤 1：连接正常关闭，服务器主动发 FIN，进入 TIME_WAIT。
        // 步骤 2：网络中有一个延迟的数据包还在路上——比如某个路由器缓存了它 30 秒才转发：
        //        这个旧包的内容：src=10.0.0.1:54321, dst=10.0.0.2:8080, seq=3000, payload="转账100元"
        // 步骤 3：如果没有 TIME_WAIT 保护，服务器立即重启，同一个客户端恰好又连上了同一对端口（四元组相同）：
        //        新连接：客户端 10.0.0.1:54321  ←→  服务器 10.0.0.2:8080
        //        序列号范围：seq=1~200
        // 步骤 4：那个迟到的旧包（seq=3000, payload="转账100元"）终于到达服务器网卡。

        // 如果没有 TIME_WAIT：内核看到四元组匹配，且 seq=3000 落在新连接的有效窗口内（TCP
        // 窗口可能恰好覆盖到这里），这个旧数据就会被当作新连接的有效载荷吞进去——应用层 read()
        // 读到一句不知道从哪来的"转账100元"
        auto SetReuseAddr(bool on) -> void;

        // 允许多个 socket 绑定到完全相同的 IP 地址和端口。内核会将传入的连接均匀分发到所有绑定了该端口的 socket
        auto SetReusePort(bool on) -> void;
        auto SetKeepAlive(bool on) -> void;

    private:
        int sockfd_;
    };
} // namespace net
