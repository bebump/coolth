#include "serial.h"

#include "windows_handles.h"

#include <tchar.h>
#include <functional>
#include <mutex>
#include <vector>

void ThrowOnFailure(bool success, const char* error_msg) {
  if (!success) {
    throw std::runtime_error(error_msg + std::string(" Reason: ") +
                             GetLastErrorAsString());
  }
}

namespace bbmp {
class Serial::Impl {
 public:
  Impl(const char* port_name,
       std::function<void(const char*, size_t)> read_callback)
      : read_callback_(std::move(read_callback)),
        read_buffer_(1024),
        read_issued_(false),
        write_buffer_(1024),
        write_issued_(false) {
    try {
      port_handle_ = WindowsHandle<-1>(
          CreateFile(port_name, GENERIC_READ | GENERIC_WRITE, 0, 0,
                     OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0));
    } catch (std::runtime_error& error) {
      throw std::runtime_error("Failed opening port " + std::string(port_name) +
                               +". Reason: " + GetLastErrorAsString());
    }

    DCB dcb;
    ZeroMemory(&dcb, sizeof(decltype(dcb)));
    ThrowOnFailure(GetCommState(port_handle_.Get(), &dcb),
                   "GetCommState failed");
    dcb.BaudRate = 19200;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    ThrowOnFailure(SetCommState(port_handle_.Get(), &dcb),
                   "SetCommState failed");
    COMMTIMEOUTS commtimeouts;
    ZeroMemory(&commtimeouts, sizeof(decltype(commtimeouts)));
    commtimeouts.ReadIntervalTimeout = 5;
    commtimeouts.ReadTotalTimeoutConstant = 5;
    commtimeouts.ReadTotalTimeoutMultiplier = 5;
    commtimeouts.WriteTotalTimeoutConstant = 5;
    commtimeouts.WriteTotalTimeoutMultiplier = 5;
    ThrowOnFailure(SetCommTimeouts(port_handle_.Get(), &commtimeouts),
                   "SetCommTimeouts failed");
    ZeroMemory(&overlapped_, sizeof(decltype(overlapped_)));
    overlapped_.hEvent = reinterpret_cast<HANDLE>(this);
    ZeroMemory(&write_overlapped_, sizeof(decltype(write_overlapped_)));
    write_overlapped_.hEvent = reinterpret_cast<HANDLE>(this);
  }

  ~Impl() {
    int result = CancelIoEx(port_handle_.Get(), &overlapped_);
    if (result != 0 || GetLastError() != ERROR_NOT_FOUND) {
      read_issued_ = true;
      while (read_issued_) {
        SleepEx(50, true);
      }
    }
  }

  void IssueRead() {
    auto lock = std::lock_guard(read_mutex_);
    if (read_issued_) {
      return;
    }
    ThrowOnFailure(ReadFileEx(port_handle_.Get(), read_buffer_.data(),
                              read_buffer_.size(), &overlapped_, ReadCallback),
                   "ReadFileEx failed");
    read_issued_ = true;
  }

  bool TryIssueWrite(const char* data, size_t length) {
    if (length > write_buffer_.size()) {
      throw std::runtime_error("IssueWrite: data larger than buffer");
    }
    auto lock = std::lock_guard(write_mutex_);
    if (write_issued_) {
      return false;
    }

    std::copy(data, data + length, write_buffer_.data());

    ThrowOnFailure(WriteFileEx(port_handle_.Get(), write_buffer_.data(), length,
                               &write_overlapped_, WriteCallback),
                   "WriteFileEx failed");

    write_issued_ = true;
    return true;
  }

 private:
  std::function<void(const char*, size_t)> read_callback_;
  WindowsHandle<-1> port_handle_;
  std::vector<char> read_buffer_;
  OVERLAPPED overlapped_;
  OVERLAPPED write_overlapped_;
  std::vector<char> write_buffer_;
  bool read_issued_;
  std::mutex read_mutex_;
  bool write_issued_;
  std::mutex write_mutex_;

  static void ReadCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
                           LPOVERLAPPED lpOverlapped) {
    auto p_this = reinterpret_cast<Impl*>(lpOverlapped->hEvent);
    auto lock = std::lock_guard(p_this->read_mutex_);
    p_this->read_callback_(p_this->read_buffer_.data(),
                           dwNumberOfBytesTransfered);
    p_this->read_issued_ = false;
  }

  static void WriteCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
                            LPOVERLAPPED lpOverlapped) {
    auto p_this = reinterpret_cast<Impl*>(lpOverlapped->hEvent);
    auto lock = std::lock_guard(p_this->write_mutex_);
    p_this->write_issued_ = false;
    //    p_this->read_callback_(p_this->read_buffer_.data(),
    //                           dwNumberOfBytesTransfered);
    //    p_this->read_issued_ = false;
  }
};

Serial::Serial(const char* port_name,
               std::function<void(const char*, size_t)> read_callback)
    : impl_(std::make_unique<Impl>(port_name, std::move(read_callback))) {}

Serial::~Serial() = default;

void Serial::IssueRead() { impl_->IssueRead(); }

bool Serial::TryIssueWrite(const char* data, size_t length) {
  return impl_->TryIssueWrite(data, length);
}

std::vector<std::string> QueryKey(HKEY hKey) {
  const int kMaxKeyLength = 255;
  const int bufferSize = 16383;

  TCHAR achKey[kMaxKeyLength];          // buffer for subkey name
  DWORD cbName;                         // size of name string
  TCHAR achClass[MAX_PATH] = TEXT("");  // buffer for class name
  DWORD cchClassName = MAX_PATH;        // size of class string
  DWORD cSubKeys = 0;                   // number of subkeys
  DWORD cbMaxSubKey;                    // longest subkey size
  DWORD cchMaxClass;                    // longest class string
  DWORD cValues;                        // number of values for key
  DWORD cchMaxValue;                    // longest value name
  DWORD cbMaxValueData;                 // longest value data
  DWORD cbSecurityDescriptor;           // size of security descriptor
  FILETIME ftLastWriteTime;             // last write time

  DWORD i, retCode;

  TCHAR achValue[bufferSize];
  DWORD cchValue = bufferSize;
  TBYTE achData[bufferSize];
  DWORD cchData = bufferSize;

  // Get the class name and the value count.
  retCode = RegQueryInfoKey(hKey,             // key handle
                            achClass,         // buffer for class name
                            &cchClassName,    // size of class string
                            NULL,             // reserved
                            &cSubKeys,        // number of subkeys
                            &cbMaxSubKey,     // longest subkey size
                            &cchMaxClass,     // longest class string
                            &cValues,         // number of values for this key
                            &cchMaxValue,     // longest value name
                            &cbMaxValueData,  // longest value data
                            &cbSecurityDescriptor,  // security descriptor
                            &ftLastWriteTime);      // last write time

  std::vector<std::string> com_ports;

  // Enumerate the key values.
  if (cValues) {
    for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++) {
      cchValue = bufferSize;
      cchData = bufferSize;
      achValue[0] = '\0';
      retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, achData,
                             &cchData);

      if (retCode == ERROR_SUCCESS && cchData > 1) {
        com_ports.emplace_back(reinterpret_cast<char*>(achData), cchData - 1);
      }
    }
  }

  return com_ports;
}

std::vector<std::string> GetComPortNames() {
  std::vector<std::string> port_names;

  try {
    HKEY hTestKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("Hardware\\DeviceMap\\SerialComm"), 0, KEY_READ,
                     &hTestKey) == ERROR_SUCCESS) {
      port_names = QueryKey(hTestKey);
    } else {
      RegCloseKey(hTestKey);
      ThrowOnFailure(false, "RegOpenKeyEx failed");
    }

    RegCloseKey(hTestKey);
  } catch (std::runtime_error& error) {
  }

  return port_names;
}

void Serial::WindowsSleepEx(uint32_t timeout_milliseconds, bool alertable) {
  SleepEx(timeout_milliseconds, alertable);
}
}  // namespace bbmp