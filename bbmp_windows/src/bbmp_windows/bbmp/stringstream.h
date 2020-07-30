#pragma once

#include <optional>

struct StringStream {
  StringStream(const char* data, size_t length)
      : data_(data), length_(length) {}

  std::optional<int> GetInt();

 private:
  const char* data_;
  size_t length_;
};
