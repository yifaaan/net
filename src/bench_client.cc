#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
using ns = std::chrono::nanoseconds;

struct Options {
  std::string ip;
  uint16_t port{};
  int threads{1};
  int conns{1};
  int seconds{10};
  int payload_bytes{16};  // message body bytes (not incl. 4-byte header)
};

static void PrintUsage(const char* argv0) {
  std::cerr
      << "Usage: " << argv0
      << " <ip> <port> [-t threads] [-c connections] [-d seconds] [-s bytes]\n"
      << "  Protocol: 4-byte little-endian length header + body, expects same reply format.\n"
      << "Examples:\n"
      << "  " << argv0 << " 127.0.0.1 5085 -t 4 -c 200 -d 15 -s 32\n";
}

static bool ParseInt(const char* s, int& out) {
  char* end = nullptr;
  long v = std::strtol(s, &end, 10);
  if (!s || *s == '\0' || (end && *end != '\0')) return false;
  if (v < INT32_MIN || v > INT32_MAX) return false;
  out = static_cast<int>(v);
  return true;
}

static bool ParseArgs(int argc, char** argv, Options& opt) {
  if (argc < 3) return false;
  opt.ip = argv[1];
  int port_i = 0;
  if (!ParseInt(argv[2], port_i) || port_i <= 0 || port_i > 65535) return false;
  opt.port = static_cast<uint16_t>(port_i);

  for (int i = 3; i < argc; i++) {
    const std::string k = argv[i];
    auto need = [&](int& v) -> bool {
      if (i + 1 >= argc) return false;
      i++;
      return ParseInt(argv[i], v);
    };
    if (k == "-t") {
      if (!need(opt.threads)) return false;
    } else if (k == "-c") {
      if (!need(opt.conns)) return false;
    } else if (k == "-d") {
      if (!need(opt.seconds)) return false;
    } else if (k == "-s") {
      if (!need(opt.payload_bytes)) return false;
    } else {
      return false;
    }
  }

  if (opt.threads <= 0) return false;
  if (opt.conns <= 0) return false;
  if (opt.seconds <= 0) return false;
  if (opt.payload_bytes <= 0) return false;
  return true;
}

static int ConnectTcp(const std::string& ip, uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
    ::close(fd);
    return -1;
  }

  if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    ::close(fd);
    return -1;
  }
  return fd;
}

static bool SendAll(int fd, const void* data, size_t len) {
  const char* p = static_cast<const char*>(data);
  size_t left = len;
  while (left > 0) {
    ssize_t n = ::send(fd, p, left, 0);
    if (n > 0) {
      p += n;
      left -= static_cast<size_t>(n);
      continue;
    }
    if (n == -1 && errno == EINTR) continue;
    return false;
  }
  return true;
}

static bool RecvAll(int fd, void* data, size_t len) {
  char* p = static_cast<char*>(data);
  size_t left = len;
  while (left > 0) {
    ssize_t n = ::recv(fd, p, left, MSG_WAITALL);
    if (n > 0) {
      p += n;
      left -= static_cast<size_t>(n);
      continue;
    }
    if (n == -1 && errno == EINTR) continue;
    return false;
  }
  return true;
}

static uint64_t Percentile(std::vector<uint64_t>& xs, double p) {
  if (xs.empty()) return 0;
  std::sort(xs.begin(), xs.end());
  double pos = (p / 100.0) * (static_cast<double>(xs.size() - 1));
  size_t idx = static_cast<size_t>(pos);
  return xs[idx];
}

struct Stats {
  uint64_t ok{0};
  uint64_t err{0};
  std::vector<uint64_t> rtt_us;
};
}  // namespace

int main(int argc, char** argv) {
  Options opt;
  if (!ParseArgs(argc, argv, opt)) {
    PrintUsage(argv[0]);
    return 2;
  }

  const int conns_per_thread = (opt.conns + opt.threads - 1) / opt.threads;
  const auto t0 = Clock::now();
  const auto deadline = t0 + std::chrono::seconds(opt.seconds);

  std::atomic<uint64_t> ok_total{0};
  std::atomic<uint64_t> err_total{0};
  std::mutex rtt_mu;
  std::vector<uint64_t> rtt_all_us;

  std::vector<std::thread> threads;
  threads.reserve(opt.threads);

  for (int ti = 0; ti < opt.threads; ti++) {
    threads.emplace_back([&, ti] {
      std::vector<int> fds;
      fds.reserve(conns_per_thread);
      for (int i = 0; i < conns_per_thread; i++) {
        int fd = ConnectTcp(opt.ip, opt.port);
        if (fd < 0) {
          err_total.fetch_add(1, std::memory_order_relaxed);
          continue;
        }
        fds.push_back(fd);
      }

      std::string body(static_cast<size_t>(opt.payload_bytes), 'x');
      std::vector<char> req(4 + body.size());
      int32_t len = static_cast<int32_t>(body.size());
      std::memcpy(req.data(), &len, 4);
      std::memcpy(req.data() + 4, body.data(), body.size());

      std::vector<uint64_t> rtt_us_local;
      rtt_us_local.reserve(1024);

      std::vector<char> hdr(4);
      while (Clock::now() < deadline) {
        for (int fd : fds) {
          auto s = Clock::now();
          if (!SendAll(fd, req.data(), req.size())) {
            err_total.fetch_add(1, std::memory_order_relaxed);
            continue;
          }

          int32_t rlen = 0;
          if (!RecvAll(fd, &rlen, 4) || rlen <= 0 || rlen > 1024 * 1024) {
            err_total.fetch_add(1, std::memory_order_relaxed);
            continue;
          }
          std::string resp(static_cast<size_t>(rlen), '\0');
          if (!RecvAll(fd, resp.data(), resp.size())) {
            err_total.fetch_add(1, std::memory_order_relaxed);
            continue;
          }

          auto e = Clock::now();
          ok_total.fetch_add(1, std::memory_order_relaxed);
          auto us = std::chrono::duration_cast<std::chrono::microseconds>(e - s).count();
          rtt_us_local.push_back(static_cast<uint64_t>(us));
        }
      }

      for (int fd : fds) ::close(fd);

      std::lock_guard<std::mutex> lk(rtt_mu);
      rtt_all_us.insert(rtt_all_us.end(), rtt_us_local.begin(), rtt_us_local.end());
    });
  }

  for (auto& th : threads) th.join();

  const auto t1 = Clock::now();
  const double sec = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();

  const uint64_t ok = ok_total.load(std::memory_order_relaxed);
  const uint64_t err = err_total.load(std::memory_order_relaxed);
  const double qps = sec > 0 ? (static_cast<double>(ok) / sec) : 0.0;

  uint64_t p50 = Percentile(rtt_all_us, 50);
  uint64_t p90 = Percentile(rtt_all_us, 90);
  uint64_t p99 = Percentile(rtt_all_us, 99);
  uint64_t maxv = rtt_all_us.empty() ? 0 : *std::max_element(rtt_all_us.begin(), rtt_all_us.end());

  std::cout << "bench_client result\n"
            << "  target: " << opt.ip << ":" << opt.port << "\n"
            << "  threads: " << opt.threads << "\n"
            << "  connections: " << opt.conns << "\n"
            << "  duration_s: " << opt.seconds << "\n"
            << "  payload_bytes: " << opt.payload_bytes << "\n"
            << "  ok: " << ok << "\n"
            << "  err: " << err << "\n"
            << "  qps: " << qps << "\n"
            << "  rtt_us: p50=" << p50 << " p90=" << p90 << " p99=" << p99 << " max=" << maxv
            << "\n";

  return 0;
}

