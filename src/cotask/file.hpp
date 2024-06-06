#pragma once

#include <cotask/cotask.hpp>

#include <span>
#include <array>
#include <string>
#include <filesystem>

namespace cotask {

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

public:
  const AsyncIoType type = AsyncIoType::FileReadBuf;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[40]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  const std::filesystem::path path;
  std::span<char> buf;
  std::uint64_t offset = 0;

public:
  FileReadBuf(TaskScheduler &ts, const std::filesystem::path &path, std::span<char> buf, std::uint64_t offset = 0);
  ~FileReadBuf();

  auto io_recived(std::uint32_t bytes_transferred) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

  inline auto await_ready() -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    cohandle.promise().is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    cohandle.promise().is_waiting = true;
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

public:
  const AsyncIoType type = AsyncIoType::FileReadAll;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[40]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  const std::filesystem::path path;
  std::array<char, 500> buf;
  std::vector<char> content;
  std::uint64_t offset = 0;

public:
  FileReadAll(TaskScheduler &ts, const std::filesystem::path &path, std::uint64_t offset = 0);
  ~FileReadAll();

  auto io_request() -> bool;
  auto io_recived(std::uint32_t bytes_transferred) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

  inline auto await_ready() -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    cohandle.promise().is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    cohandle.promise().is_waiting = true;
  }

  [[nodiscard]] inline auto await_resume() const noexcept -> FileReadAllResult {
    return {
      .finished = finished,
      .success = success,
      .content = content,
    };
  }
};

} // namespace cotask
