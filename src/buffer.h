#pragma once

#include <string>
class Buffer {
 public:
  Buffer();
  ~Buffer();

  // 给buffer添加数据
  void Append(const char* data, size_t len);
  void AppendWithHead(const char* data, size_t len);
  size_t size() const { return buf_.size(); }
  const char* data() const { return buf_.data(); }
  void Clear() { buf_.clear(); }

  void Erase(size_t pos, size_t n) { buf_.erase(pos, n); }

 private:
  std::string buf_;
};