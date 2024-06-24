#pragma once

#include <cotask/cotask.hpp>

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

public:
  explicit Timer(std::uint64_t timeout);
  ~Timer();

public:
  auto start() -> void;
  auto close() -> void;

  inline auto on_ended() -> void {
    if (is_waiting != nullptr) {
      *is_waiting = false;
    }
  }
};

} // namespace cotask
