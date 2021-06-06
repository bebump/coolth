#pragma once

#include <optional>

struct StringStream {
  StringStream(const char* data, size_t length)
      : data_(data), length_(length) {}

  std::optional<int> GetInt();
  void ForwardToNextToken();

 private:
  const char* data_;
  size_t length_;
};
