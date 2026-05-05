#include "buffer.h"

Buffer::Buffer() = default;

Buffer::~Buffer() = default;

void Buffer::Append(const char* data, size_t len) { buf_.append(data, len); }

void Buffer::AppendWithHead(const char* data, size_t len) {
  int size = len;
  buf_.append(reinterpret_cast<char*>(&size), sizeof(size));
  buf_.append(data, size);
}