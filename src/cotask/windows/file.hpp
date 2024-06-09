#pragma once

#include <cotask/file.hpp>

#include <winsock2.h>
#include <windows.h>

namespace cotask {

struct OverlappedFile : public OVERLAPPED {
  const FileIoType type;
};

struct OverlappedFileReadBuf : OVERLAPPED {
  const FileIoType type = FileIoType::ReadBuf;
  FileReadBuf *read_buf;

  inline explicit OverlappedFileReadBuf(FileReadBuf *read_buf) : OVERLAPPED{}, read_buf{read_buf} {}
};

struct FileReadBuf::Impl {
  OverlappedFileReadBuf ovex;

  inline explicit Impl(FileReadBuf *read_buf) : ovex{read_buf} {}
};

struct OverlappedFileReadAll : OVERLAPPED {
  const FileIoType type = FileIoType::ReadAll;
  FileReadAll *read_all;

  inline OverlappedFileReadAll(FileReadAll *read_all) : OVERLAPPED{}, read_all{read_all} {}
};

struct FileReadAll::Impl {
  OverlappedFileReadAll ovex;

  inline explicit Impl(FileReadAll *read_all) : ovex{read_all} {}
};

struct FileReader::Impl {
  HANDLE file_handle = nullptr;
};

} // namespace cotask
