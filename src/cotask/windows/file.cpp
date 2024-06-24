#include "cotask.hpp"
#include "file.hpp"

#include <cotask/impl.hpp>
#include <cotask/utils.hpp>

#include <iostream>

namespace cotask {

FileReadBuf::FileReadBuf(TaskScheduler &ts, FileReader *reader, std::span<char> buf, std::uint64_t offset)
    : ts{ts}, reader{reader}, buf{buf}, offset{offset} {
  IMPL_CONSTRUCT(this);

  // setup OVERLAPPED
  impl->ovex.Offset = static_cast<std::uint32_t>(offset);           // low 32bits
  impl->ovex.OffsetHigh = static_cast<std::uint32_t>(offset >> 32); // high 32bits

  // read file
  auto read_success = ::ReadFile(reader->impl->file_handle, buf.data(), static_cast<DWORD>(buf.size()),
                                 reinterpret_cast<DWORD *>(&bytes_read), &impl->ovex);
  const auto err_code = ::GetLastError();
  if (not read_success and err_code != ERROR_IO_PENDING) {
    auto path_str = std::filesystem::absolute(reader->path).string();
    std::cerr << utils::with_location(std::format("ReadFile failed for \"{}\": {}", path_str, err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return;
  }

  success = true;
}

FileReadBuf::~FileReadBuf() {
  std::destroy_at(impl);
}

auto FileReadBuf::io_read(std::uint32_t bytes_read) -> void {
  // check finished
  if (bytes_read <= buf.size()) {
    if (is_waiting != nullptr) {
      *is_waiting = false;
    }
    finished = true;
    buf = {buf.data(), bytes_read};
  }
}

auto FileReadBuf::io_failed(std::uint32_t err_code) -> void {
  if (is_waiting != nullptr) {
    *is_waiting = false;
  }
  finished = true;
  success = false;

  std::cerr << utils::with_location(std::format("FileReadToBuf compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

FileReadAll::FileReadAll(TaskScheduler &ts, FileReader *reader, std::size_t offset)
    : ts{ts}, reader{reader}, offset{offset} {
  IMPL_CONSTRUCT(this);

  // setup OVERLAPPED
  impl->ovex.Offset = static_cast<std::uint32_t>(offset);           // low 32bits
  impl->ovex.OffsetHigh = static_cast<std::uint32_t>(offset >> 32); // high 32bits

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
  auto read_success = ::ReadFile(reader->impl->file_handle, buf.data(), static_cast<DWORD>(buf.size()),
                                 reinterpret_cast<DWORD *>(&bytes_read), &impl->ovex);
  const auto err_code = ::GetLastError();
  if (not read_success and err_code != ERROR_IO_PENDING) {
    success = false;

    auto path_str = std::filesystem::absolute(reader->path).string();
    std::cerr << utils::with_location(std::format("ReadFile failed for \"{}\": {}", path_str, err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  return true;
}

auto FileReadAll::io_read(std::uint32_t bytes_read) -> void {
  // accumulate bytes
  offset += bytes_read;
  impl->ovex.Offset = static_cast<std::uint32_t>(offset);           // low 32bits
  impl->ovex.OffsetHigh = static_cast<std::uint32_t>(offset >> 32); // high 32bits
  content.insert(content.end(), buf.data(), buf.data() + bytes_read);

  // check finished
  if (bytes_read < buf.size()) {
    if (is_waiting != nullptr) {
      *is_waiting = false;
    }
    finished = true;
    success = true;
    return;
  }

  // read more bytes
  if (not io_request()) {
    if (is_waiting != nullptr) {
      *is_waiting = false;
    }
  }
}

auto FileReadAll::io_failed(std::uint32_t err_code) -> void {
  if (is_waiting != nullptr) {
    *is_waiting = false;
  }
  finished = true;
  success = false;

  std::cerr << utils::with_location(std::format("FileReadAll compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

FileReader::FileReader(TaskScheduler &ts, const std::filesystem::path &path) : ts{ts}, path{path} {
  IMPL_CONSTRUCT();

  // open file
  impl->file_handle =
    ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

  if (impl->file_handle == nullptr) {
    const auto err_code = ::GetLastError();

    auto path_str = std::filesystem::absolute(path).string();
    std::cerr << utils::with_location(std::format("CreateFileW failed for \"{}\": {}", path_str, err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return;
  }

  // setup IOCP
  if (::CreateIoCompletionPort(impl->file_handle, ts.impl->iocp_handle, (ULONG_PTR)this, 0) == nullptr) {
    const auto err_code = ::GetLastError();
    ::CloseHandle(impl->file_handle);
    impl->file_handle = nullptr;

    auto path_str = std::filesystem::absolute(path).string();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed for \"{}\": {}", path_str, err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  }
}

FileReader::~FileReader() {
  std::destroy_at(impl);
}

auto FileReader::close() -> void {
  if (impl->file_handle != nullptr) {
    ::CloseHandle(impl->file_handle);
  }
}

} // namespace cotask
