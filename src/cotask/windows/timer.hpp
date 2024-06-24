#pragma once

#include <cotask/timer.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace cotask {

struct Timer::Impl {
  PTP_TIMER timer = nullptr;
  FILETIME due_time{};
};

} // namespace cotask
