#include "child_process.h"

#include "windows_handles.h"

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

#define NOMINMAX
#define NOGDI
#include <strsafe.h>
#include <windows.h>
#undef NOMINMAX
#undef NOGDI

class ChildProcess::Impl {
 public:
  Impl(const std::string& path_to_exe,
       std::function<void(const char*, size_t)> read_callback)
      : read_callback_(std::move(read_callback)), read_issued_(false) {
    const auto pipe_name = std::string("\\\\.\\pipe\\") +
                           std::string(std::to_string(GetCurrentProcessId()));

    SECURITY_ATTRIBUTES security_attributes;
    ZeroMemory(&security_attributes, sizeof(security_attributes));
    security_attributes.nLength = sizeof(security_attributes);
    security_attributes.lpSecurityDescriptor = NULL;
    security_attributes.bInheritHandle = FALSE;

    // Read handle for the parent process
    read_handle_ = WindowsHandle<-1>(CreateNamedPipeA(
        pipe_name.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, 0, 1,
        8192, 8192, 0, &security_attributes));

    // Write handle for the child process
    // Zero out and re-use security attributes with inherit flag enabled
    ZeroMemory(&security_attributes, sizeof(security_attributes));
    security_attributes.nLength = sizeof(security_attributes);
    security_attributes.lpSecurityDescriptor = NULL;
    security_attributes.bInheritHandle = TRUE;

    write_handle_ = WindowsHandle<-1>(
        CreateFileA(pipe_name.c_str(), GENERIC_WRITE, 0, &security_attributes,
                    OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));

    // Event handle for ConnectNamedPipe
    ZeroMemory(&overlapped_, sizeof(overlapped_));
    overlapped_.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (overlapped_.hEvent == NULL) {
      throw std::runtime_error("CreateEvent failed");
    }

    ScopeGuard overlapped_hEvent_guard;
    overlapped_hEvent_guard.Add([this]() { CloseHandle(overlapped_.hEvent); });

    // Connect to the pipe
    BOOL success = ConnectNamedPipe(read_handle_.Get(), &overlapped_);
    if (!success) {
      if (GetLastError() == ERROR_IO_PENDING) {
        if (WaitForSingleObject(overlapped_.hEvent, INFINITE) == WAIT_FAILED) {
          CloseHandle(overlapped_.hEvent);
          throw std::runtime_error("failed WSFO for ConnectNamedPipe");
        }
      } else if (GetLastError() != ERROR_PIPE_CONNECTED) {
        CloseHandle(overlapped_.hEvent);
        throw std::runtime_error("failed to connect to named pipe");
      }
    }
    CloseHandle(overlapped_.hEvent);
    overlapped_hEvent_guard.CancelAll();

    // Spawn the new process
    PROCESS_INFORMATION process_information;
    STARTUPINFO startupinfo;
    ZeroMemory(&process_information, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startupinfo, sizeof(STARTUPINFO));
    startupinfo.cb = sizeof(STARTUPINFO);
    // Not passing stdin handle. Feel free to add it if you need it.
    startupinfo.hStdError = write_handle_.Get();
    startupinfo.hStdOutput = write_handle_.Get();
    startupinfo.dwFlags = STARTF_USESTDHANDLES;

    const int command_buffer_size = 2048;
    char command_buffer[command_buffer_size];
    strncpy(command_buffer, path_to_exe.c_str(), command_buffer_size);
    command_buffer[command_buffer_size - 1] = '\n';

    success =
        CreateProcess(NULL, command_buffer, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                      0, NULL, &startupinfo, &process_information);
    if (!success) {
      throw std::runtime_error(std::string("CreateProcess failed with: ") +
                               command_buffer);
    }

    process_handle_ = WindowsHandle<-1>(process_information.hProcess);
    thread_handle_ = WindowsHandle<-1>(process_information.hThread);

    // We reuse the overlapped_ structure for read() calls
    ZeroMemory(&overlapped_, sizeof(decltype(overlapped_)));
    overlapped_.hEvent = reinterpret_cast<HANDLE>(this);
  }

  void IssueRead() {
    auto lock = std::lock_guard(read_mutex_);
    if (read_issued_) {
      return;
    }
    auto success = ReadFileEx(read_handle_.Get(), read_buffer_.data(),
                              read_buffer_.size(), &overlapped_, ReadCallback);
    if (!success) {
      throw std::runtime_error("ReadFileEx failed");
    }

    read_issued_ = true;
  }

  ~Impl() {
    int result = CancelIoEx(read_handle_.Get(), &overlapped_);
    if (result != 0 || GetLastError() != ERROR_NOT_FOUND) {
      read_issued_ = true;
      while (read_issued_) {
        SleepEx(50, true);
      }
    }
  }

 private:
  std::function<void(const char*, size_t)> read_callback_;
  std::array<char, 1024> read_buffer_{};
  OVERLAPPED overlapped_;
  WindowsHandle<-1> read_handle_;
  WindowsHandle<-1> write_handle_;
  WindowsHandle<-1> process_handle_;
  WindowsHandle<-1> thread_handle_;
  bool read_issued_;
  std::mutex read_mutex_;

  static void ReadCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
                           LPOVERLAPPED lpOverlapped) {
    auto p_this = reinterpret_cast<Impl*>(lpOverlapped->hEvent);
    auto lock = std::lock_guard(p_this->read_mutex_);
    p_this->read_callback_(p_this->read_buffer_.data(),
                           dwNumberOfBytesTransfered);
    p_this->read_issued_ = false;
  }
};

ChildProcess::ChildProcess(
    const std::string& path_to_exe,
    std::function<void(const char*, size_t)> read_callback)
    : impl_(std::make_unique<Impl>(path_to_exe, std::move(read_callback))) {}

ChildProcess::~ChildProcess() = default;

void ChildProcess::IssueRead() { impl_->IssueRead(); }

void WindowsSleepEx(uint32_t timeout_milliseconds, bool alertable) {
  SleepEx(timeout_milliseconds, alertable);
}
