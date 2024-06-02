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
  while (not async_tasks.empty() or waiting_task_count != 0) {
    // https://jacking75.github.io/info-design-issues-when-using-iocp-in-a-winsock-server/
    auto entries = std::array<OVERLAPPED_ENTRY, 10>{};
    auto num_entries = ULONG{};
    auto timeout = async_tasks.empty() ? 500 : 0;
    if (not ::GetQueuedCompletionStatusEx(impl->iocp_handle, entries.data(), entries.size(), &num_entries, timeout,
                                          FALSE)) {
      const auto err_code = ::GetLastError();
      if (err_code != WAIT_TIMEOUT) {
        std::cerr << utils::with_location(std::format("GetQueuedCompletionStatusEx failed: {}", err_code))
                  << std::format("err msg: {}\n", std::system_category().message((int)err_code));
        return;
      }
    }

    // handle io completions
    for (const auto entry : std::span{entries.data(), num_entries}) {
      const auto completion_key = std::bit_cast<AsyncIoBase *>(entry.lpCompletionKey);
      const auto overlapped = entry.lpOverlapped;
      const auto bytes_transferred = entry.dwNumberOfBytesTransferred;

      assert(completion_key != nullptr);
      assert(overlapped != nullptr);

      auto n = DWORD{};
      switch (completion_key->type) {
      case AsyncIoType::FileReadBuf: {
        auto io_task = (FileReadBuf *)completion_key;
        if (not ::GetOverlappedResult(io_task->impl->file_handle, entry.lpOverlapped, &n, FALSE)) {
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
        if (not ::GetOverlappedResult(io_task->impl->file_handle, entry.lpOverlapped, &n, FALSE)) {
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
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, entry.lpOverlapped, &n, FALSE, &flags)) {
            const auto err_code = ::GetLastError();
            if (err_code != ERROR_HANDLE_EOF) {
              ovex->io_failed(err_code);
              continue;
            }
          }
          ovex->io_succeed();
        } break;

        case TcpIoType::Connect: {
          auto ovex = reinterpret_cast<OverlappedTcpConnect *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, entry.lpOverlapped, &n, FALSE, &flags)) {
            const auto err_code = ::GetLastError();
            if (err_code != ERROR_HANDLE_EOF) {
              ovex->io_failed(err_code);
              continue;
            }
          }
          ovex->io_succeed();
        } break;

        case TcpIoType::RecvOnce: {
          auto ovex = reinterpret_cast<OverlappedTcpRecvOnce *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, entry.lpOverlapped, &n, FALSE, &flags)) {
            const auto err_code = ::GetLastError();
            if (err_code != ERROR_HANDLE_EOF) {
              ovex->io_failed(err_code);
              continue;
            }
          }
          ovex->io_succeed(bytes_transferred);
        } break;

        case TcpIoType::RecvAll: {
          auto ovex = reinterpret_cast<OverlappedTcpRecvAll *>(ov);
          // TODO
        } break;

        case TcpIoType::SendOnce: {
          auto ovex = reinterpret_cast<OverlappedTcpSendOnce *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, entry.lpOverlapped, &n, FALSE, &flags)) {
            const auto err_code = ::GetLastError();
            if (err_code != ERROR_HANDLE_EOF) {
              ovex->io_failed(err_code);
              continue;
            }
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

    // run tasks
    if (not async_tasks.empty()) {
      auto task = async_tasks.front();
      async_tasks.pop_front();
      if (task.can_resume()) {
        task.resume();
        if (not task.done()) {
          async_tasks.push_back(task);
        }
      }

      // destroy ended coroutine handles
      for (auto &task : ended_task | std::views::reverse) {
        task.destroy();
      }
      ended_task.clear();
    }
  }

  // event loop ended
  assert(ended_task.empty());

  // destroy ended coroutine handles
  for (auto &task : from_sync_tasks | std::views::reverse) {
    task.destroy();
  }
  from_sync_tasks.clear();
}

} // namespace cotask
