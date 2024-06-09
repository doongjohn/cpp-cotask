#pragma once

#include <cotask/cotask.hpp>

#include <windows.h>

namespace cotask {

struct OverlappedTcp : public OVERLAPPED {
  const TcpIoType type;
};

struct TaskScheduler::Impl {
  HANDLE iocp_handle = nullptr;
};

} // namespace cotask
