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

  [[nodiscard]] auto to_string() const -> std::string;
  [[nodiscard]] auto to_string_view() const -> std::string_view;
};

struct FileReadBuf {
  friend TaskScheduler;

private:
  struct Impl;
  uint8_t impl_storage[50]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

  const std::filesystem::path path;
  std::span<char> buf;
  std::size_t offset;

public:
  FileReadBuf(TaskScheduler &ts, const std::filesystem::path &path, std::span<char> buf, std::size_t offset = 0);
  ~FileReadBuf();

  auto io_recived(uint32_t bytes_transferred) -> void;
  auto io_failed(uint32_t err_code) -> void;

  inline auto await_ready() -> bool {
    return finished or not success;
  }

  template <typename T>
  auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
    ts.add_waiting_task_count();
  }

  auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
    ts.add_waiting_task_count();
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

  [[nodiscard]] auto to_string() const -> std::string;
  [[nodiscard]] auto to_string_view() const -> std::string_view;
};

struct FileReadAll {
  friend TaskScheduler;

private:
  struct Impl;
  uint8_t impl_storage[50]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

  const std::filesystem::path path;
  std::array<char, 500> buf;
  std::vector<char> content;
  std::size_t offset;

public:
  FileReadAll(TaskScheduler &ts, const std::filesystem::path &path, std::size_t offset = 0);
  ~FileReadAll();

  auto io_request() -> bool;
  auto io_recived(uint32_t bytes_transferred) -> void;
  auto io_failed(uint32_t err_code) -> void;

  inline auto await_ready() -> bool {
    return finished or not success;
  }

  template <typename T>
  auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
    ts.add_waiting_task_count();
  }

  auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
    ts.add_waiting_task_count();
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