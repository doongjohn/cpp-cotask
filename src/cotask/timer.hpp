#pragma once

#include <cotask/cotask.hpp>

#include <functional>

namespace cotask {

struct Timer {
public:
  const AsyncIoType type = AsyncIoType::Timer;

public:
  struct Impl;
  alignas(8) std::uint8_t impl_storage[16]{};
  Impl *impl;

public:
  std::uint64_t timeout; // milliseconds
  bool ended = false;
  bool *is_waiting;
  std::function<void()> fn_on_ended;

public:
  explicit Timer(std::uint64_t timeout);
  ~Timer();

public:
  auto start() -> void;
  auto close() -> void;

  inline auto on_ended() -> void {
    ended = true;
    if (is_waiting != nullptr) {
      *is_waiting = false;
    }
    if (fn_on_ended) {
      fn_on_ended();
    }
  }
};

} // namespace cotask
