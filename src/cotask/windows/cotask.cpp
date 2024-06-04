#include "file.hpp"
#include "tcp.hpp"
#include <cotask/impl.hpp>
#include <cotask/utils.hpp>

#include <cassert>
#include <array>
#include <ranges>
#include <iostream>

namespace cotask {

auto init() -> void {
  auto wsa_data = WSADATA{};
  auto startup_result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (startup_result != 0) {
    std::cerr << utils::with_location(std::format("WSAStartup failed: {}", startup_result))
              << std::format("err msg: {}\n", std::system_category().message((int)startup_result));
  }
}

auto deinit() -> void {
  if (::WSACleanup() != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSACleanup failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  }
}

TaskScheduler::TaskScheduler() {
  CONSTRUCT_IMPL();

  impl->iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
  if (impl->iocp_handle == nullptr) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  }
}

TaskScheduler::~TaskScheduler() {
  if (impl->iocp_handle != nullptr) {
    ::CloseHandle(impl->iocp_handle);
  }
  std::destroy_at(impl);
}

auto TaskScheduler::execute() -> void {
  // event loop
  while (not async_tasks.empty()) {
    // resume task
    auto task = async_tasks.front();
    async_tasks.pop_front();
    if (task.can_resume()) {
      task.resume();
    }
    if (not task.done()) {
      async_tasks.push_back(task);
    }

    // destroy ended coroutine handles
    for (auto &cohandle : ended_task | std::views::reverse) {
      cohandle.destroy();
    }
    ended_task.clear();

    if (async_tasks.empty()) {
      // TODO: handle remaining io completions
      break;
    }

    auto entries = std::array<OVERLAPPED_ENTRY, 10>{};
    auto num_entries = ULONG{};
    if (not ::GetQueuedCompletionStatusEx(impl->iocp_handle, entries.data(), static_cast<ULONG>(entries.size()),
                                          &num_entries, 0, FALSE)) {
      const auto err_code = ::GetLastError();
      if (err_code == WAIT_TIMEOUT) {
        continue;
      } else {
        std::cerr << utils::with_location(std::format("GetQueuedCompletionStatusEx failed: {}", err_code))
                  << std::format("err msg: {}\n", std::system_category().message((int)err_code));
        return;
      }
    }

    // handle io completions
    for (const auto entry : std::span{entries.data(), num_entries}) {
      const auto completion_key = std::bit_cast<AsyncIoBase *>(entry.lpCompletionKey);
      if (completion_key == nullptr) {
        continue;
      }

      assert(impl->io_task_count != 0);
      if (impl->io_task_count != 0) {
        impl->io_task_count -= 1;
      }

      const auto overlapped = entry.lpOverlapped;
      const auto bytes_transferred = entry.dwNumberOfBytesTransferred;

      auto n = DWORD{};
      switch (completion_key->type) {
      case AsyncIoType::FileReadBuf: {
        auto io_task = (FileReadBuf *)completion_key;
        if (not ::GetOverlappedResult(io_task->impl->file_handle, overlapped, &n, TRUE)) {
          const auto err_code = ::GetLastError();
          if (err_code != ERROR_HANDLE_EOF) {
            io_task->io_failed(err_code);
            continue;
          }
        }
        io_task->io_recived(bytes_transferred);
      } break;

      case AsyncIoType::FileReadAll: {
        auto io_task = (FileReadAll *)completion_key;
        if (not ::GetOverlappedResult(io_task->impl->file_handle, overlapped, &n, TRUE)) {
          const auto err_code = ::GetLastError();
          if (err_code != ERROR_HANDLE_EOF) {
            io_task->io_failed(err_code);
            continue;
          }
        }
        io_task->io_recived(bytes_transferred);
      } break;

      case AsyncIoType::TcpSocket: {
        auto tcp_socket = reinterpret_cast<TcpSocket *>(completion_key);
        auto ov = reinterpret_cast<TcpOverlapped *>(overlapped);
        auto flags = DWORD{};

        switch (ov->type) {
        case TcpIoType::Accept: {
          auto ovex = reinterpret_cast<OverlappedTcpAccept *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_succeed();
        } break;

        case TcpIoType::Connect: {
          auto ovex = reinterpret_cast<OverlappedTcpConnect *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_succeed();
        } break;

        case TcpIoType::RecvOnce: {
          auto ovex = reinterpret_cast<OverlappedTcpRecvOnce *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_succeed(bytes_transferred);
        } break;

        case TcpIoType::RecvAll: {
          auto ovex = reinterpret_cast<OverlappedTcpRecvAll *>(ov);
          // TODO
        } break;

        case TcpIoType::SendOnce: {
          auto ovex = reinterpret_cast<OverlappedTcpSendOnce *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_succeed(bytes_transferred);
        } break;

        case TcpIoType::SendAll: {
          auto ovex = reinterpret_cast<OverlappedTcpSendAll *>(ov);
          // TODO
        } break;
        }
      } break;
      }
    }
  }

  // destroy ended coroutine handles
  for (auto &cohandle : from_sync_tasks | std::views::reverse) {
    cohandle.destroy();
  }
  from_sync_tasks.clear();
}

} // namespace cotask
