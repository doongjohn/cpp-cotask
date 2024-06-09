#pragma once

#include <cotask/cotask.hpp>

#include <span>
#include <array>
#include <string>
#include <filesystem>

namespace cotask {

struct FileReader;

struct FileReadBufResult {
  bool finished = false;
  bool success = false;

  std::span<char> buf;

  [[nodiscard]] inline auto get_string() const -> std::string {
    return {buf.data(), buf.size()};
  }

  [[nodiscard]] inline auto get_string_view() const -> std::string_view {
    return {buf.data(), buf.size()};
  }
};

struct FileReadBuf {
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

private:
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  FileReader *reader;
  std::span<char> buf;
  std::uint32_t bytes_read = 0;
  std::uint64_t offset = 0;

public:
  FileReadBuf(TaskScheduler &ts, FileReader *reader, std::span<char> buf, std::uint64_t offset = 0);
  inline FileReadBuf(const FileReadBuf &other) = delete;
  ~FileReadBuf();

  auto io_read(std::uint32_t bytes_read) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

  inline auto await_ready() -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  [[nodiscard]] inline auto await_resume() const noexcept -> FileReadBufResult {
    return {
      .finished = finished,
      .success = success,
      .buf = buf,
    };
  }
};

struct FileReadAllResult {
  bool finished = false;
  bool success = false;

  std::vector<char> content;

  [[nodiscard]] inline auto get_string() const -> std::string {
    return {content.data(), content.size()};
  }

  [[nodiscard]] inline auto get_string_view() const -> std::string_view {
    return {content.data(), content.size()};
  }
};

struct FileReadAll {
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

private:
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  FileReader *reader;
  std::array<char, 500> buf;
  std::uint32_t bytes_read = 0;
  std::uint64_t offset = 0;
  std::vector<char> content;

public:
  FileReadAll(TaskScheduler &ts, FileReader *reader, std::uint64_t offset = 0);
  inline FileReadAll(const FileReadAll &other) = delete;
  ~FileReadAll();

  auto io_request() -> bool;
  auto io_read(std::uint32_t bytes_transferred) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

  inline auto await_ready() -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  [[nodiscard]] inline auto await_resume() const noexcept -> FileReadAllResult {
    return {
      .finished = finished,
      .success = success,
      .content = content,
    };
  }
};

struct FileReader {
public:
  const AsyncIoType type = AsyncIoType::FileRead;

public:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[8]{};
  Impl *impl;

public:
  TaskScheduler &ts;
  const std::filesystem::path path;

public:
  FileReader(TaskScheduler &ts, const std::filesystem::path &path);
  ~FileReader();

public:
  inline auto read_buf(std::span<char> buf, std::size_t offset = 0) -> FileReadBuf {
    return {ts, this, buf, offset};
  }

  inline auto read_all(std::size_t offset = 0) -> FileReadAll {
    return {ts, this, offset};
  }

  auto close() -> void;
};

} // namespace cotask
