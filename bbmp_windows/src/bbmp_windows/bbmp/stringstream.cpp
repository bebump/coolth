#include "stringstream.h"

std::optional<int> StringStream::GetInt() {
  bool found_an_integer = false;
  int integer = 0;
  bool negative = false;

  while (length_ > 0 && (*data_ == ' ')) {
    ++data_;
    --length_;
  }

  if (*data_ == '-') {
    negative = true;
    ++data_;
    --length_;
  }

  while (length_ > 0 && *data_ >= '0' && *data_ <= '9') {
    integer *= 10;
    integer += *data_ - '0';
    ++data_;
    --length_;
    found_an_integer = true;
  }

  if (found_an_integer) {
    return negative ? -integer : integer;
  }

  return {};
}
