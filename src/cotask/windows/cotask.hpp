#pragma once

#include <cotask/cotask.hpp>

#include <winsock2.h>
#include <windows.h>

namespace cotask {

enum struct AsyncIoType {
  FileReadBuf,
  FileReadAll,
  TcpSocket,
};

struct AsyncIoBase {
  const AsyncIoType type;
};

enum struct TcpIoType {
  Accept,
  Connect,
  RecvOnce,
  RecvAll,
  SendOnce,
  SendAll,
};

struct TcpOverlapped {
  OVERLAPPED ov{};
  const TcpIoType type;
};

struct TaskScheduler::Impl {
  HANDLE iocp_handle = nullptr;
};

} // namespace cotask
