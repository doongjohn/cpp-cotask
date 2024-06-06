#pragma once

#include <cotask/cotask.hpp>

#include <winsock2.h>
#include <windows.h>

namespace cotask {

struct OverlappedTcp : public OVERLAPPED {
  const TcpIoType type;
};

struct TaskScheduler::Impl {
  HANDLE iocp_handle = nullptr;
};

} // namespace cotask
