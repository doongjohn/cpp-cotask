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
  const AsyncIoType type = AsyncIoType::TcpSocket;

public:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[16]{};
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
  auto listen(std::uint16_t port) -> bool;
  auto close() -> bool;
};

struct TcpAcceptResult {
  bool finished = false;
  bool success = false;

  TcpAcceptResult(bool finished, bool success, TcpSocket *accept_socket);
};

struct TcpAccept {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;
  TcpSocket *accept_socket;

public:
  TcpAccept(TcpSocket *sock, TcpSocket *accept_socket);
  inline TcpAccept(const TcpAccept &other) = delete;
  ~TcpAccept();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  inline auto await_resume() -> TcpAcceptResult {
    return {finished, success, accept_socket};
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
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

public:
  TcpConnect(TcpSocket *sock, std::string_view ip, std::string_view port);
  inline TcpConnect(const TcpConnect &other) = delete;
  ~TcpConnect();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
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

  [[nodiscard]] inline auto get_string() const -> std::string {
    return {buf.data(), buf.size()};
  }

  [[nodiscard]] inline auto get_string_view() const -> std::string_view {
    return {buf.data(), buf.size()};
  }
};

struct TcpRecvOnce {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;
  std::uint32_t bytes_received = 0;

public:
  TcpRecvOnce(TcpSocket *sock, std::span<char> buf);
  inline TcpRecvOnce(const TcpRecvOnce &other) = delete;
  ~TcpRecvOnce();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  inline auto await_resume() -> TcpRecvResult {
    return {
      .finished = finished,
      .success = success,
      .buf = {buf.data(), bytes_received},
    };
  }
};

struct TcpRecvAll {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;

public:
  TcpRecvAll(TcpSocket *sock, std::span<char> buf);
  inline TcpRecvAll(const TcpRecvAll &other) = delete;
  ~TcpRecvAll();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
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
  std::uint32_t bytes_sent = 0;
};

struct TcpSendOnce {
  friend TcpSocket;
  friend TaskScheduler;

public:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<const char> buf;
  std::uint32_t bytes_sent = 0;

public:
  TcpSendOnce(TcpSocket *sock, std::span<const char> buf);
  inline TcpSendOnce(const TcpSendOnce &other) = delete;
  ~TcpSendOnce();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
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
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  std::coroutine_handle<> cohandle = nullptr;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;
  std::uint32_t bytes_sent = 0;

public:
  TcpSendAll(TcpSocket *sock, std::span<char> buf);
  ~TcpSendAll();

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->cohandle = cohandle;
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
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
