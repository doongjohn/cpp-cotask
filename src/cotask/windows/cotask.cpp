#include "cotask.hpp"
#include "file.hpp"
#include "tcp.hpp"

#include <cotask/impl.hpp>
#include <cotask/utils.hpp>

#include <cassert>
#include <array>
#include <ranges>
#include <iostream>

namespace cotask {

auto net_init() -> void {
  auto wsa_data = WSADATA{};
  auto startup_result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (startup_result != 0) {
    std::cerr << utils::with_location(std::format("WSAStartup failed: {}", startup_result))
              << std::format("err msg: {}\n", std::system_category().message((int)startup_result));
  }
}

auto net_deinit() -> void {
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
  constexpr auto max_count = 10lu;
  auto entries = std::array<OVERLAPPED_ENTRY, max_count>{};
  auto num_entries = ULONG{};

  // event loop
  while (not tasks.empty()) {
    // resume a task (round-robin)
    auto task = tasks.front();
    tasks.pop_front();
    if (task.can_resume()) {
      task.resume();
    }
    if (not task.done()) {
      tasks.push_back(task);
    }

    // destroy ended coroutines
    for (auto &cohandle : ended_task | std::views::reverse) {
      cohandle.destroy();
    }
    ended_task.clear();

    // check io compeletions
    if (not ::GetQueuedCompletionStatusEx(impl->iocp_handle, entries.data(), max_count, &num_entries, 0, FALSE)) {
      const auto err_code = ::GetLastError();
      if (err_code == WAIT_TIMEOUT) {
        continue;
      } else {
        std::cerr << utils::with_location(std::format("GetQueuedCompletionStatusEx failed: {}", err_code))
                  << std::format("err msg: {}\n", std::system_category().message((int)err_code));
        return;
      }
    }

    // handle io compeletions
    for (const auto entry : std::span{entries.data(), num_entries}) {
      if (entry.lpCompletionKey == 0) {
        return;
      }

      const auto completion_key = std::bit_cast<AsyncIoBase *>(entry.lpCompletionKey);
      const auto overlapped = entry.lpOverlapped;
      const auto bytes_transferred = entry.dwNumberOfBytesTransferred;

      auto n = DWORD{};
      switch (std::bit_cast<AsyncIoBase *>(completion_key)->type) {
      case AsyncIoType::FileRead: {
        auto reader = std::bit_cast<FileReader *>(completion_key);
        auto ov = reinterpret_cast<OverlappedFile *>(overlapped);

        switch (ov->type) {
        case FileIoType::ReadBuf: {
          auto ovex = reinterpret_cast<OverlappedFileReadBuf *>(ov);
          if (not ::GetOverlappedResult(reader->impl->file_handle, overlapped, &n, TRUE)) {
            const auto err_code = ::GetLastError();
            if (err_code != ERROR_HANDLE_EOF) {
              ovex->read_buf->io_failed(err_code);
              continue;
            }
          }
          ovex->read_buf->io_read(bytes_transferred);
        } break;

        case FileIoType::ReadAll: {
          auto ovex = reinterpret_cast<OverlappedFileReadAll *>(ov);
          if (not ::GetOverlappedResult(reader->impl->file_handle, overlapped, &n, TRUE)) {
            const auto err_code = ::GetLastError();
            if (err_code != ERROR_HANDLE_EOF) {
              ovex->read_all->io_failed(err_code);
              continue;
            }
          }
          ovex->read_all->io_read(bytes_transferred);
        } break;
        }
      } break;

      case AsyncIoType::FileWrite: {
        // TODO
      } break;

      case AsyncIoType::TcpSocket: {
        auto tcp_socket = std::bit_cast<TcpSocket *>(completion_key);
        auto ov = reinterpret_cast<OverlappedTcp *>(overlapped);
        auto flags = DWORD{};

        switch (ov->type) {
        case TcpIoType::Accept: {
          auto ovex = reinterpret_cast<OverlappedTcpAccept *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_received(bytes_transferred);
        } break;

        case TcpIoType::Connect: {
          auto ovex = reinterpret_cast<OverlappedTcpConnect *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_received(bytes_transferred);
        } break;

        case TcpIoType::Recv: {
          auto ovex = reinterpret_cast<OverlappedTcpRecv *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_received(bytes_transferred);
        } break;

        case TcpIoType::RecvAll: {
          // TODO
          // auto ovex = reinterpret_cast<OverlappedTcpRecvAll *>(ov);
        } break;

        case TcpIoType::Send: {
          auto ovex = reinterpret_cast<OverlappedTcpSend *>(ov);
          if (not ::WSAGetOverlappedResult(tcp_socket->impl->socket, overlapped, &n, TRUE, &flags)) {
            const auto err_code = ::GetLastError();
            ovex->io_failed(err_code);
            continue;
          }
          ovex->io_sent(bytes_transferred);
        } break;

        case TcpIoType::SendAll: {
          // TODO
          // auto ovex = reinterpret_cast<OverlappedTcpSendAll *>(ov);
        } break;
        }
      } break;
      }
    }
  }

  // destroy top level coroutines
  for (auto &cohandle : top_level_tasks | std::views::reverse) {
    cohandle.destroy();
  }
  top_level_tasks.clear();
}

} // namespace cotask
