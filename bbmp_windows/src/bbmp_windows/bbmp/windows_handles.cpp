#include "windows_handles.h"

std::string GetLastErrorAsString() {
  char buf[512];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf,
                 (sizeof(buf) / sizeof(char)), NULL);
  return std::string(buf);
}
