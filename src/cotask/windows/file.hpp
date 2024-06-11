#pragma once

#include <cotask/file.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace cotask {

struct OverlappedFile : public OVERLAPPED {
  const FileIoType type;
};

} // namespace cotask

namespace cotask {

struct FileReader::Impl {
  HANDLE file_handle = nullptr;
};

struct OverlappedFileReadBuf : OVERLAPPED {
  const FileIoType type = FileIoType::ReadBuf;
  FileReadBuf *awaitable;

  inline explicit OverlappedFileReadBuf(FileReadBuf *read_buf) : OVERLAPPED{}, awaitable{read_buf} {}
};

struct FileReadBuf::Impl {
  OverlappedFileReadBuf ovex;

  inline explicit Impl(FileReadBuf *awaitable) : ovex{awaitable} {}
};

struct OverlappedFileReadAll : OVERLAPPED {
  const FileIoType type = FileIoType::ReadAll;
  FileReadAll *awaitable;

  inline OverlappedFileReadAll(FileReadAll *read_all) : OVERLAPPED{}, awaitable{read_all} {}
};

struct FileReadAll::Impl {
  OverlappedFileReadAll ovex;

  inline explicit Impl(FileReadAll *awaitable) : ovex{awaitable} {}
};

} // namespace cotask
