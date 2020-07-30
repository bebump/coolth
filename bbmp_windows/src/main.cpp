#include "bbmp/serial.h"

#include <iostream>

int main() {
  try {
    bbmp::Serial serial("COM3", [](const char* data, size_t length) {
      std::cout << std::string(data, length);
    });
    for (int i = 0; i < 5000 / 100; ++i) {
      serial.IssueRead();
      bbmp::Serial::WindowsSleepEx(100, true);
    }

    const char* danaj = "c 1 100 100 100 100\n";
    serial.TryIssueWrite(danaj, strlen(danaj));

    for (int i = 0; i < 5000 / 100; ++i) {
      serial.IssueRead();
      bbmp::Serial::WindowsSleepEx(100, true);
    }
  } catch (std::runtime_error& error) {
    std::cout << error.what() << std::endl;
  }
}
