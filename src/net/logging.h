// C++20 lightweight logging wrapper for the muduo tree
// Provides stream-style logging with RAII flush and simple macros LOG20_*.
#pragma once

#include <chrono>
#include <format>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <source_location>

namespace net
{

    enum class LogLevel
    {
        kTrace,
        kDebug,
        kInfo,
        kWarn,
        kError,
        kFatal
    };

    class Logger
    {
    public:
        Logger(const std::source_location& loc = std::source_location::current(), LogLevel level = LogLevel::kInfo) :
            file_(std::filesystem::path(loc.file_name()).filename().string()),
            line_(static_cast<int>(loc.line())),
            func_(loc.function_name()),
            level_(level)
        {
        }

        ~Logger()
        {
            flush();
        }

        std::ostringstream& stream() { return oss_; }

        static void SetLogLevel(LogLevel) {}
        static LogLevel Level() { return LogLevel::kInfo; }

    private:
        void flush()
        {
            using namespace std::chrono;
            auto now = system_clock::now();
            auto secs = floor<seconds>(now);
            auto us = duration_cast<microseconds>(now.time_since_epoch()).count() % 1000000LL;

            std::string time_str = std::format("{:%Y%m%d %H:%M:%S}", secs);
            std::ostringstream tid_ss;
            tid_ss << std::this_thread::get_id();
            std::string thread_str = tid_ss.str();

            static const char* names[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
            std::string lvl = names[static_cast<int>(level_)];

            std::string header = std::format("{}.{:06} [{}] {} {}:{} - ", time_str, static_cast<long>(us), thread_str, lvl, file_, line_);

            std::string msg = oss_.str();

            static std::mutex mutex;
            std::lock_guard lock(mutex);
            std::cout << header << msg << '\n';
            std::cout.flush();
            if (level_ == LogLevel::kFatal)
            {
                std::abort();
            }
        }

        std::string file_;
        int line_;
        std::string func_;
        LogLevel level_;
        std::ostringstream oss_;
    };

} // namespace net

#define LOG_TRACE                                      \
    if (net::Logger::Level() <= net::LogLevel::kTrace) \
    net::Logger(std::source_location::current(), net::LogLevel::kTrace).stream()
#define LOG_DEBUG                                      \
    if (net::Logger::Level() <= net::LogLevel::kDebug) \
    net::Logger(std::source_location::current(), net::LogLevel::kDebug).stream()
#define LOG_INFO \
    net::Logger(std::source_location::current(), net::LogLevel::kInfo).stream()
#define LOG_WARN \
    net::Logger(std::source_location::current(), net::LogLevel::kWarn).stream()
#define LOG_ERROR \
    net::Logger(std::source_location::current(), net::LogLevel::kError).stream()
#define LOG_FATAL \
    net::Logger(std::source_location::current(), net::LogLevel::kFatal).stream()
