#include "line_reader.h"

void LineReader::ProcessLine(const char* data, size_t length) {
  line_processor_(data, length);
}

void LineReader::Read(const char* data, size_t length) {
  if (!valid_) {
    for (size_t i = 0; i < length; ++i) {
      if (data[i] == '\n') {
        valid_ = true;
        data += (i + 1);
        length -= (i + 1);
        break;
      }
    }
  }

  for (size_t i = 0; i < length; ++i) {
    if (data[i] == '\n') {
      ProcessLine(buffer_.data(), length_);

      length_ = 0;
    } else {
      if (length_ >= buffer_.size()) {
        throw std::runtime_error(
            "The buffer was filled before the token terminated");
      }
      buffer_.data()[length_++] = data[i];
    }
  }
}
