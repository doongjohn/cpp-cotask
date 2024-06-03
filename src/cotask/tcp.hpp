#pragma once

#include <cotask/cotask.hpp>

#include <span>
#include <string_view>

namespace cotask {

struct TcpAcceptResult;
struct TcpAccept;

struct TcpConnectResult;
struct TcpConnect;

struct TcpRecvResult;
struct TcpRecvOnce;
struct TcpRecvAll;

struct TcpSendResult;
struct TcpSendOnce;
struct TcpSendAll;

} // namespace cotask

namespace cotask {

struct TcpSocket {
  friend TaskScheduler;

public:
  struct Impl;
  uint8_t impl_storage[24]{};
  Impl *impl;

  TaskScheduler &ts;

public:
  TcpSocket(TaskScheduler &ts);
  TcpSocket(const TcpSocket &other);
  ~TcpSocket();

public:
  inline auto operator==(const TcpSocket &other) const -> bool {
    return std::memcmp(impl_storage, other.impl_storage, sizeof(impl_storage)) == 0 and ts == other.ts;
  }

  auto operator=(const TcpSocket &other) -> TcpSocket &;

public:
  auto bind(uint16_t port) -> bool;
  auto listen() -> bool;
  auto accept() -> TcpAccept;
  auto connect(std::string_view ip, std::string_view port) -> TcpConnect;
  auto close() -> bool;

  auto recv_once(std::span<char> buf) -> TcpRecvOnce;
  auto recv_all(std::span<char> buf) -> TcpRecvAll;

  auto send_once(std::span<char> buf) -> TcpSendOnce;
  auto send_all(std::span<char> buf) -> TcpSendAll;
};

struct TcpAcceptResult {
  bool finished = false;
  bool success = false;
  TcpSocket accept_socket;
};

struct TcpAccept {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  uint8_t impl_storage[48]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

  TcpSocket accept_socket;

public:
  TcpAccept(TaskScheduler &ts);
  ~TcpAccept();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    // return finished or not success;
    return false;
  }

  template <typename T>
  inline auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    if (not finished and success) {
      this->cohandle = cohandle;
      await_subtask = &cohandle.promise().await_subtask;
      cohandle.promise().await_subtask = true;
    }
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    if (not finished and success) {
      this->cohandle = cohandle;
      await_subtask = &cohandle.promise().await_subtask;
      cohandle.promise().await_subtask = true;
    }
  }

  inline auto await_resume() -> TcpAcceptResult {
    return {
      .finished = finished,
      .success = success,
      .accept_socket = accept_socket,
    };
  }
};

struct TcpConnectResult {
  bool finished = false;
  bool success = false;
};

struct TcpConnect {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  uint8_t impl_storage[48]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

public:
  TcpConnect(TaskScheduler &ts);
  ~TcpConnect();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename T>
  inline auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
  }

  inline auto await_resume() -> TcpConnectResult {
    return {
      .finished = finished,
      .success = success,
    };
  }
};

struct TcpRecvResult {
  bool finished = false;
  bool success = false;
  std::span<char> buf;
};

struct TcpRecvOnce {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  uint8_t impl_storage[48]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;

public:
  TcpRecvOnce(TaskScheduler &ts, std::span<char> buf);
  ~TcpRecvOnce();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    // return finished or not success;
    return false;
  }

  template <typename T>
  inline auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    if (not finished and success) {
      this->cohandle = cohandle;
      await_subtask = &cohandle.promise().await_subtask;
      cohandle.promise().await_subtask = true;
    }
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    if (not finished and success) {
      this->cohandle = cohandle;
      await_subtask = &cohandle.promise().await_subtask;
      cohandle.promise().await_subtask = true;
    }
  }

  inline auto await_resume() -> TcpRecvResult {
    return {
      .finished = finished,
      .success = success,
      .buf = buf,
    };
  }
};

struct TcpRecvAll {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  uint8_t impl_storage[48]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;

public:
  TcpRecvAll(TaskScheduler &ts, std::span<char> buf);
  ~TcpRecvAll();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename T>
  inline auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
  }

  inline auto await_resume() -> TcpRecvResult {
    return {
      .finished = finished,
      .success = success,
      .buf = buf,
    };
  }
};

struct TcpSendResult {
  bool finished = false;
  bool success = false;
  uint32_t bytes_sent = 0;
};

struct TcpSendOnce {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  uint8_t impl_storage[48]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;
  uint32_t bytes_sent = 0;

public:
  TcpSendOnce(TaskScheduler &ts, std::span<char> buf);
  ~TcpSendOnce();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    // return finished or not success;
    return false;
  }

  template <typename T>
  inline auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    if (not finished and success) {
      this->cohandle = cohandle;
      await_subtask = &cohandle.promise().await_subtask;
      cohandle.promise().await_subtask = true;
    }
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    if (not finished and success) {
      this->cohandle = cohandle;
      await_subtask = &cohandle.promise().await_subtask;
      cohandle.promise().await_subtask = true;
    }
  }

  inline auto await_resume() -> TcpSendResult {
    return {
      .finished = finished,
      .success = success,
      .bytes_sent = bytes_sent,
    };
  }
};

struct TcpSendAll {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  uint8_t impl_storage[48]{};
  Impl *impl;

  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *await_subtask = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;
  uint32_t bytes_sent = 0;

public:
  TcpSendAll(TaskScheduler &ts, std::span<char> buf);
  ~TcpSendAll();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename T>
  inline auto await_suspend(std::coroutine_handle<typename Task<T>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    await_subtask = &cohandle.promise().await_subtask;
    cohandle.promise().await_subtask = true;
  }

  inline auto await_resume() -> TcpSendResult {
    return {
      .finished = finished,
      .success = success,
      .bytes_sent = bytes_sent,
    };
  }
};

} // namespace cotask
