#include "file.hpp"
#include <cotask/impl.hpp>
#include <cotask/utils.hpp>

#include <iostream>

namespace cotask {

[[nodiscard]] auto FileReadBufResult::to_string() const -> std::string {
  return {buf.data(), buf.size()};
}

[[nodiscard]] auto FileReadBufResult::to_string_view() const -> std::string_view {
  return {buf.data(), buf.size()};
}

FileReadBuf::FileReadBuf(TaskScheduler &ts, const std::filesystem::path &path, std::span<char> buf, uint64_t offset)
    : ts{ts}, path{path}, buf{buf}, offset{offset} {
  CONSTRUCT_IMPL();

  // initialize OVERLAPPED
  impl->ov.Offset = static_cast<uint32_t>(offset);           // low 32bits
  impl->ov.OffsetHigh = static_cast<uint32_t>(offset >> 32); // high 32bits

  // open file
  impl->file_handle =
    ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

  if (impl->file_handle == nullptr) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(
                   std::format("CreateFileW failed for \"{}\": {}", std::filesystem::absolute(path).string(), err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return;
  }

  // initialize IOCP
  if (::CreateIoCompletionPort(impl->file_handle, ts.impl->iocp_handle, (ULONG_PTR)this, 0) == nullptr) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed for \"{}\": {}",
                                                  std::filesystem::absolute(path).string(), err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::CloseHandle(impl->file_handle);
    return;
  }

  // read file
  auto bytes_read = DWORD{};
  const auto read_success =
    ::ReadFile(impl->file_handle, buf.data(), static_cast<DWORD>(buf.size()), &bytes_read, &impl->ov);
  const auto read_err_code = ::GetLastError();
  if (not read_success and read_err_code != ERROR_IO_PENDING) {
    std::cerr << utils::with_location(std::format("ReadFile failed for \"{}\": {}",
                                                  std::filesystem::absolute(path).string(), read_err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)read_err_code));
    ::CloseHandle(impl->file_handle);
    return;
  }

  ts.impl->io_task_count += 1;
  success = true;
}

FileReadBuf::~FileReadBuf() {
  std::destroy_at(impl);
}

auto FileReadBuf::io_recived(uint32_t bytes_transferred) -> void {
  // check finished
  if (bytes_transferred <= buf.size()) {
    ::CloseHandle(impl->file_handle);
    finished = true;
    if (await_subtask != nullptr) {
      *await_subtask = false;
    }
  }
}

auto FileReadBuf::io_failed(uint32_t err_code) -> void {
  std::cerr << utils::with_location(std::format("FileReadToBuf compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));

  ::CloseHandle(impl->file_handle);
  success = false;
  if (await_subtask != nullptr) {
    *await_subtask = false;
  }
}

[[nodiscard]] auto FileReadAllResult::to_string() const -> std::string {
  return {content.data(), content.size()};
}

[[nodiscard]] auto FileReadAllResult::to_string_view() const -> std::string_view {
  return {content.data(), content.size()};
}

FileReadAll::FileReadAll(TaskScheduler &ts, const std::filesystem::path &path, std::size_t offset)
    : ts{ts}, path{path}, offset{offset} {
  CONSTRUCT_IMPL();

  // initialize OVERLAPPED
  impl->ov.Offset = static_cast<uint32_t>(offset);           // low 32bits
  impl->ov.OffsetHigh = static_cast<uint32_t>(offset >> 32); // high 32bits

  // open file
  impl->file_handle =
    ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

  if (impl->file_handle == nullptr) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(
                   std::format("CreateFileW failed for \"{}\": {}", std::filesystem::absolute(path).string(), err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return;
  }

  // initialize IOCP
  if (::CreateIoCompletionPort(impl->file_handle, ts.impl->iocp_handle, (ULONG_PTR)this, 0) == nullptr) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed for \"{}\": {}",
                                                  std::filesystem::absolute(path).string(), err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::CloseHandle(impl->file_handle);
    return;
  }

  // read file
  if (not io_request()) {
    return;
  }

  success = true;
}

FileReadAll::~FileReadAll() {
  std::destroy_at(impl);
}

auto FileReadAll::io_request() -> bool {
  auto bytes_read = DWORD{};
  const auto read_success =
    ::ReadFile(impl->file_handle, buf.data(), static_cast<DWORD>(buf.size()), &bytes_read, &impl->ov);
  const auto err_code = ::GetLastError();
  if (not read_success and err_code != ERROR_IO_PENDING) {
    std::cerr << utils::with_location(
                   std::format("ReadFile failed for \"{}\": {}", std::filesystem::absolute(path).string(), err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::CloseHandle(impl->file_handle);
    success = false;
    return false;
  }

  ts.impl->io_task_count += 1;
  return true;
}

auto FileReadAll::io_recived(uint32_t bytes_transferred) -> void {
  // append bytes
  offset += bytes_transferred;
  impl->ov.Offset = static_cast<uint32_t>(offset);           // low 32bits
  impl->ov.OffsetHigh = static_cast<uint32_t>(offset >> 32); // high 32bits
  content.insert(content.end(), buf.data(), buf.data() + bytes_transferred);

  // check finished
  if (bytes_transferred < buf.size()) {
    ::CloseHandle(impl->file_handle);
    finished = true;
    if (await_subtask != nullptr) {
      *await_subtask = false;
    }
    return;
  }

  // read more bytes
  if (not io_request()) {
    if (await_subtask != nullptr) {
      *await_subtask = false;
    }
  }
}

auto FileReadAll::io_failed(uint32_t err_code) -> void {
  std::cerr << utils::with_location(std::format("FileReadAll compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));

  ::CloseHandle(impl->file_handle);
  success = false;
  if (await_subtask != nullptr) {
    *await_subtask = false;
  }
}

} // namespace cotask
