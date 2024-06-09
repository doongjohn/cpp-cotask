#pragma once

#include <cotask/cotask.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace cotask {

struct OverlappedTcp : public OVERLAPPED {
  const TcpIoType type;
};

struct TaskScheduler::Impl {
  HANDLE iocp_handle = nullptr;
};

} // namespace cotask
