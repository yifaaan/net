#pragma once

#include <string_view>

#include <sys/types.h>

namespace net::base::CurrentThread
{
    // Linux 线程 id，按线程缓存一次，后续直接复用。
    [[nodiscard]] auto Tid() noexcept -> pid_t;

    // 线程 id 的字符串形式，适合日志场景。
    [[nodiscard]] auto TidString() noexcept -> std::string_view;

    // 当前线程是否为主线程。
    [[nodiscard]] auto IsMainThread() noexcept -> bool;
} // namespace net::base::CurrentThread
