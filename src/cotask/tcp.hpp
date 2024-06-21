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
struct TcpRecv;
struct TcpRecvAll;

struct TcpSendResult;
struct TcpSend;
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
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[144]{};
  Impl *impl;

public:
  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;
  TcpSocket *accept_socket;

public:
  TcpAccept(TcpSocket *sock, TcpSocket *accept_socket);
  inline TcpAccept(const TcpAccept &other) = delete;
  ~TcpAccept();

public:
  auto io_received(std::uint32_t bytes_received) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
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
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

public:
  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

public:
  TcpConnect(TcpSocket *sock, std::string_view ip, std::string_view port);
  inline TcpConnect(const TcpConnect &other) = delete;
  ~TcpConnect();

public:
  auto io_received(std::uint32_t bytes_received) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
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

struct TcpRecv {
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

public:
  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;
  std::uint32_t bytes_received = 0;

public:
  TcpRecv(TcpSocket *sock, std::span<char> buf);
  inline TcpRecv(const TcpRecv &other) = delete;
  ~TcpRecv();

public:
  auto io_received(std::uint32_t bytes_received) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
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
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

public:
  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<char> buf;
  std::uint32_t bytes_received = 0;
  std::size_t total_bytes_received = 0;

public:
  TcpRecvAll(TcpSocket *sock, std::span<char> buf);
  inline TcpRecvAll(const TcpRecvAll &other) = delete;
  ~TcpRecvAll();

public:
  auto io_request() -> bool;
  auto io_received(std::uint32_t bytes_received) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
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

struct TcpSend {
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

public:
  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<const char> buf;
  std::uint32_t bytes_sent = 0;

public:
  TcpSend(TcpSocket *sock, std::span<const char> buf);
  inline TcpSend(const TcpSend &other) = delete;
  ~TcpSend();

public:
  auto io_sent(std::uint32_t bytes_sent) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
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
  friend TaskScheduler;

private:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[48]{};
  Impl *impl;

public:
  TcpSocket &tcp_socket;
  TaskScheduler &ts;
  bool *is_waiting = nullptr;

  bool finished = false;
  bool success = false;

  std::span<const char> buf;
  std::uint32_t bytes_sent = 0;
  std::size_t total_bytes_sent = 0;

public:
  TcpSendAll(TcpSocket *sock, std::span<const char> buf);
  ~TcpSendAll();

public:
  auto io_request() -> bool;
  auto io_sent(std::uint32_t bytes_sent) -> void;
  auto io_failed(std::uint32_t err_code) -> void;

public:
  [[nodiscard]] inline auto await_ready() const -> bool {
    return finished or not success;
  }

  template <typename TaskResult, typename Promise = Task<TaskResult>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
    this->is_waiting = &cohandle.promise().is_waiting;
    *this->is_waiting = true;
  }

  template <typename Promise = Task<void>::promise_type>
  inline auto await_suspend(std::coroutine_handle<Promise> cohandle) noexcept -> void {
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
