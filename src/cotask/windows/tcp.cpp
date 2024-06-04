#include "tcp.hpp"
#include <cotask/impl.hpp>
#include <cotask/utils.hpp>

#include <iostream>

#include <ws2tcpip.h>

// TcpSocket
namespace cotask {

TcpSocket::TcpSocket(TaskScheduler &ts) : ts{ts} {
  CONSTRUCT_IMPL();
}

TcpSocket::TcpSocket(const TcpSocket &other) : ts{other.ts} {
  COPY_IMPL(*other.impl);
}

TcpSocket::~TcpSocket() {
  std::destroy_at(impl);
}

} // namespace cotask

// Bind
namespace cotask {

auto TcpSocket::bind(uint16_t port) -> bool {
  impl->socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  if (impl->socket == INVALID_SOCKET) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSASocketW failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  auto addr = SOCKADDR_IN{};
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
  addr.sin_port = ::htons(port);

  if (::bind(impl->socket, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("socket bind failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  return true;
}

} // namespace cotask

// Listen
namespace cotask {

auto TcpSocket::listen() -> bool {
  if (not ::CreateIoCompletionPort(std::bit_cast<HANDLE>(impl->socket), ts.impl->iocp_handle, (ULONG_PTR)this, 0)) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(impl->socket);
    return false;
  }

  if (::listen(impl->socket, SOMAXCONN) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("listen failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  return true;
}

} // namespace cotask

// Accept
namespace cotask {

auto OverlappedTcpAccept::io_succeed() -> void {
  awaitable->finished = true;
  awaitable->success = true;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }
}

auto OverlappedTcpAccept::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }

  std::cerr << utils::with_location(std::format("TcpAccept compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpAccept::TcpAccept(TaskScheduler &ts) : ts{ts}, accept_socket{ts} {
  CONSTRUCT_IMPL(this);
}

TcpAccept::~TcpAccept() {
  std::destroy_at(impl);
}

auto TcpSocket::accept() -> TcpAccept {
  auto awaitable = TcpAccept{ts};

  // create socket
  auto accept_socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  if (accept_socket == INVALID_SOCKET) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSASocketW failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return awaitable;
  }

  // accept
  // FIXME: life time of awaitable.impl->ovex must be valid until the io is complete
  alignas(8) uint8_t addr_buf[88]{};
  auto bytes_recived = DWORD{};
  auto accept_success = ::AcceptEx(impl->socket, accept_socket, addr_buf, 0, sizeof(addr_buf) / 2, sizeof(addr_buf) / 2,
                                   &bytes_recived, &awaitable.impl->ovex);
  if (not accept_success) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("AcceptEx failed: {}", ::WSAGetLastError()))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return awaitable;
    }
  }

  awaitable.success = true;
  awaitable.accept_socket.impl->socket = accept_socket;

  ts.impl->io_task_count += 1;
  return awaitable;
}

} // namespace cotask

// Connect
namespace cotask {

auto OverlappedTcpConnect::io_succeed() -> void {
  awaitable->finished = true;
  awaitable->success = true;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }
}

auto OverlappedTcpConnect::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }

  std::cerr << utils::with_location(std::format("TcpConnect compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpConnect::TcpConnect(TaskScheduler &ts) : ts{ts} {
  CONSTRUCT_IMPL(this);
}

TcpConnect::~TcpConnect() {
  std::destroy_at(impl);
}

auto TcpSocket::connect(std::string_view ip, std::string_view port) -> TcpConnect {
  auto awaitable = TcpConnect{ts};

  // TODO: Do I need to make a new socket for each connection?
  impl->socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  if (impl->socket == INVALID_SOCKET) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSASocketW failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return awaitable;
  }

  if (impl->fnConnectEx == nullptr) {
    auto guid = GUID WSAID_CONNECTEX;
    auto bytes = DWORD{};
    auto fn_load_result = ::WSAIoctl(impl->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),
                                     &impl->fnConnectEx, sizeof(impl->fnConnectEx), &bytes, nullptr, nullptr);
    if (fn_load_result != 0) {
      const auto err_code = ::WSAGetLastError();
      std::cerr << utils::with_location(std::format("ConnectEx load failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      ::closesocket(impl->socket);
      return awaitable;
    }
  }

  // bind addr
  auto addr = sockaddr_in{};
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
  addr.sin_port = ::htons(0);

  if (::bind(impl->socket, (sockaddr *)&addr, sizeof(addr)) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("socket bind failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return awaitable;
  }

  // resolve the server address and port
  auto connect_addr_list = PADDRINFOA{};
  auto addr_hints = addrinfo{};
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;

  auto getaddr_result = ::getaddrinfo(ip.data(), port.data(), &addr_hints, &connect_addr_list);
  if (getaddr_result != 0) {
    std::cerr << std::format("getaddrinfo failed: {}\n", ::gai_strerrorA(getaddr_result));
    return awaitable;
  }

  // setup iocp
  if (not ::CreateIoCompletionPort(std::bit_cast<HANDLE>(impl->socket), ts.impl->iocp_handle, (ULONG_PTR)this, 0)) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(impl->socket);
    return awaitable;
  }

  // connect
  // FIXME: life time of awaitable.impl->ovex must be valid until the io is complete
  auto connect_success = impl->fnConnectEx(impl->socket, connect_addr_list->ai_addr, sizeof(sockaddr), nullptr, 0,
                                           nullptr, &awaitable.impl->ovex);
  ::freeaddrinfo(connect_addr_list);
  if (not connect_success) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("ConnectEx failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return awaitable;
    }
  }

  awaitable.success = true;

  ts.impl->io_task_count += 1;
  return awaitable;
}

} // namespace cotask

// Close
namespace cotask {

auto TcpSocket::close() -> bool {
  if (::shutdown(impl->socket, SD_BOTH) != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSAENOTCONN) {
      std::cerr << utils::with_location(std::format("shutdown failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return false;
    }
  }

  struct linger linger = {0, 0};
  if (::setsockopt(impl->socket, SOL_SOCKET, SO_LINGER, (char *)(&linger), sizeof linger) != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSAENOTCONN) {
      std::cerr << utils::with_location(std::format("setsockopt failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return false;
    }
  }

  if (::closesocket(impl->socket) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("closesocket failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  impl->socket = INVALID_SOCKET;
  return true;
}

auto TcpSocket::assoc_iocp() -> bool {
  if (not ::CreateIoCompletionPort(std::bit_cast<HANDLE>(impl->socket), ts.impl->iocp_handle, (ULONG_PTR)this, 0)) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(impl->socket);
    return false;
  }
  return true;
}

} // namespace cotask

// RecvOnce
namespace cotask {

auto OverlappedTcpRecvOnce::io_succeed(DWORD bytes_recived) -> void {
  awaitable->finished = true;
  awaitable->success = true;
  awaitable->buf = {awaitable->buf.data(), bytes_recived};

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }
}

auto OverlappedTcpRecvOnce::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }

  std::cerr << utils::with_location(std::format("TcpRecvOnce compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpRecvOnce::TcpRecvOnce(TaskScheduler &ts, std::span<char> buf) : ts{ts}, buf{buf} {
  CONSTRUCT_IMPL(this);
}

TcpRecvOnce::~TcpRecvOnce() {
  std::destroy_at(impl);
}

auto TcpSocket::recv_once(std::span<char> buf) -> TcpRecvOnce {
  auto awaitable = TcpRecvOnce{ts, buf};

  std::cout << "bufsize: " << buf.size() << '\n';
  auto wsa_buf = WSABUF{
    .len = static_cast<ULONG>(buf.size()),
    .buf = buf.data(),
  };
  auto flags = ULONG{};

  // FIXME: life time of awaitable.impl->ovex must be valid until the io is complete
  auto recv_result =
    ::WSARecv(impl->socket, &wsa_buf, 1ul, (PULONG)&awaitable.bytes_received, &flags, &awaitable.impl->ovex, nullptr);
  if (recv_result != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("WSARecv failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return awaitable;
    }
  }

  awaitable.success = true;
  awaitable.buf = {buf.data(), awaitable.bytes_received};

  ts.impl->io_task_count += 1;
  return awaitable;
}

} // namespace cotask

// RecvAll
namespace cotask {

auto OverlappedTcpRecvAll::io_recived(DWORD bytes_recived) -> void {
  // TOOD
}

auto OverlappedTcpRecvAll::io_request() -> bool {
  // TODO
  return true;
}

auto OverlappedTcpRecvAll::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }

  std::cerr << utils::with_location(std::format("TcpRecvAll compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpRecvAll::TcpRecvAll(TaskScheduler &ts, std::span<char> buf) : ts{ts}, buf{buf} {
  CONSTRUCT_IMPL(this);
}

TcpRecvAll::~TcpRecvAll() {
  std::destroy_at(impl);
}

auto TcpSocket::recv_all(std::span<char> buf) -> TcpRecvAll {
  auto awaitable = TcpRecvAll{ts, buf};
  // TODO

  awaitable.success = true;

  ts.impl->io_task_count += 1;
  return awaitable;
}

} // namespace cotask

// SendOnce
namespace cotask {

auto OverlappedTcpSendOnce::io_succeed(DWORD bytes_sent) -> void {
  awaitable->finished = true;
  awaitable->success = true;
  awaitable->bytes_sent = bytes_sent;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }
}

auto OverlappedTcpSendOnce::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;

  if (awaitable->await_subtask != nullptr) {
    *awaitable->await_subtask = false;
  }

  std::cerr << utils::with_location(std::format("TcpSendOnce compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpSendOnce::TcpSendOnce(TaskScheduler &ts, std::span<char> buf) : ts{ts}, buf{buf} {
  CONSTRUCT_IMPL(this);
}

TcpSendOnce::~TcpSendOnce() {
  std::destroy_at(impl);
}

auto TcpSocket::send_once(std::span<char> buf) -> TcpSendOnce {
  auto awaitable = TcpSendOnce{ts, buf};

  auto wsa_buf = WSABUF{
    .len = static_cast<ULONG>(buf.size()),
    .buf = buf.data(),
  };

  // FIXME: life time of awaitable.impl->ovex must be valid until the io is complete
  auto send_result =
    ::WSASend(impl->socket, &wsa_buf, 1ul, (PULONG)&awaitable.bytes_sent, 0, &awaitable.impl->ovex, nullptr);
  if (send_result != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("WSASend failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return awaitable;
    }
  }

  awaitable.success = true;

  ts.impl->io_task_count += 1;
  return awaitable;
}

} // namespace cotask

// SendAll
namespace cotask {

auto OverlappedTcpSendAll::io_sent(DWORD bytes_sent) -> void {
  // TODO
}

auto OverlappedTcpSendAll::io_request() -> bool {
  // TODO
  return true;
}

auto OverlappedTcpSendAll::io_failed(DWORD err_code) -> void {
  // TODO
}

TcpSendAll::TcpSendAll(TaskScheduler &ts, std::span<char> buf) : ts{ts}, buf{buf} {
  CONSTRUCT_IMPL(this);
}

TcpSendAll::~TcpSendAll() {
  std::destroy_at(impl);
}

auto TcpSocket::send_all(std::span<char> buf) -> TcpSendAll {
  auto awaitable = TcpSendAll{ts, buf};
  // TODO

  awaitable.success = true;

  ts.impl->io_task_count += 1;
  return awaitable;
}

} // namespace cotask
