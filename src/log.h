#pragma once

#include <atomic>
#include <charconv>
#include <cstdint>
#include <ctime>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/pattern_formatter.h>

namespace net {
namespace detail {

class LogSequenceFlag final : public spdlog::custom_flag_formatter {
 public:
  void format(const spdlog::details::log_msg&,
              const std::tm&,
              spdlog::memory_buf_t& dest) override {
    char buffer[32]{};
    auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), Next());
    if (ec != std::errc{}) return;

    for (auto len = end - buffer; len < 8; ++len) {
      dest.push_back('0');
    }
    dest.append(buffer, end);
  }

  std::unique_ptr<spdlog::custom_flag_formatter> clone() const override {
    return std::make_unique<LogSequenceFlag>();
  }

 private:
  static uint64_t Next() {
    static std::atomic_uint64_t sequence{0};
    return sequence.fetch_add(1, std::memory_order_relaxed) + 1;
  }
};

}  // namespace detail

inline void InitLog() {
  static const bool initialized = [] {
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<detail::LogSequenceFlag>('Q')
        .set_pattern(
            "seq=%Q time=%Y-%m-%d %H:%M:%S.%e pid=%P tid=%t level=%-5l | %v");

    spdlog::set_formatter(std::move(formatter));
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::err);
    return true;
  }();
  (void)initialized;
}

}  // namespace net
