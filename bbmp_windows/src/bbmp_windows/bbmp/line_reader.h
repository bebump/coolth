#pragma once

#include <atomic>
#include <functional>
#include <optional>
#include <vector>

class LineReader {
 public:
  LineReader(
      size_t buffer_length,
      std::function<void(const char* data, size_t length)> line_processor)
      : buffer_(buffer_length),
        length_(0),
        line_processor_(std::move(line_processor)),
        valid_(false) {}

  void Read(const char* data, size_t length);

 private:
  std::vector<char> buffer_;
  size_t length_;
  std::function<void(const char* data, size_t length)> line_processor_;

  // We process messages in between two endline chars. The state of this machine
  // is considered valid, if we've encountered the first newline.
  bool valid_;

  void ProcessLine(const char* data, size_t length);
};
