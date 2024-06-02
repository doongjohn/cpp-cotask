#pragma once

#include "cotask.hpp"
#include <cotask/file.hpp>

#include <winsock2.h>
#include <windows.h>

namespace cotask {

struct FileReadBuf::Impl {
  const AsyncIoType type = AsyncIoType::FileReadBuf;

  HANDLE file_handle = nullptr;
  OVERLAPPED ov{};
};

struct FileReadAll::Impl {
  const AsyncIoType type = AsyncIoType::FileReadAll;

  HANDLE file_handle = nullptr;
  OVERLAPPED ov{};
};

} // namespace cotask
