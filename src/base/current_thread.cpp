#include "current_thread.h"

#include <string>

#include <sys/syscall.h>
#include <unistd.h>

namespace
{
    auto GetTid() noexcept -> pid_t
    {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    thread_local const pid_t kTid = GetTid();  // 按线程缓存一次，后续直接复用。
    thread_local const std::string kTidString = std::to_string(kTid);
} // namespace

namespace net::base::CurrentThread
{
    auto Tid() noexcept -> pid_t
    {
        return kTid;
    }

    auto TidString() noexcept -> std::string_view
    {
        return kTidString;
    }

    auto IsMainThread() noexcept -> bool
    {
        return Tid() == static_cast<pid_t>(::getpid());
    }
} // namespace net::base::CurrentThread
