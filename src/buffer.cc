#include "buffer.h"

Buffer::Buffer() = default;

Buffer::~Buffer() = default;

void Buffer::Append(const char* data, size_t len) { buf_.append(data, len); }