#pragma once

#include <cotask/file.hpp>

#include <winsock2.h>
#include <windows.h>

namespace cotask {

struct FileReadBuf::Impl {
  HANDLE file_handle = nullptr;
  OVERLAPPED ov{};
};

struct FileReadAll::Impl {
  HANDLE file_handle = nullptr;
  OVERLAPPED ov{};
};

} // namespace cotask
