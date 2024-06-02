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

auto TcpSocket::operator=(const TcpSocket &other) -> TcpSocket & {
  if (this == &other) {
    return *this;
  }
  *impl = *other.impl;
  ts = other.ts;
  return *this;
}

} // namespace cotask

// TcpBind
namespace cotask {

auto TcpSocket::bind(uint16_t port) -> bool {
  impl->socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  if (impl->socket == INVALID_SOCKET) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSASocketW failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  // SO_CONDITIONAL_ACCEPT
  // https://programmingdiary.tistory.com/4

  auto addr = sockaddr_in{};
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
  addr.sin_port = ::htons(port);

  if (::bind(impl->socket, (sockaddr *)&addr, sizeof(addr)) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("socket bind failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

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

// TcpListen
namespace cotask {

auto TcpSocket::listen() -> bool {
  if (::listen(impl->socket, SOMAXCONN) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("listen failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  return true;
}

} // namespace cotask

// TcpAccept
namespace cotask {

auto OverlappedTcpAccept::io_succeed() -> void {
  awaitable->finished = true;
  awaitable->success = true;

  auto socket = awaitable->accept_socket.impl->socket;
  auto iocp_handle = awaitable->ts.impl->iocp_handle;
  if (not ::CreateIoCompletionPort(std::bit_cast<HANDLE>(socket), iocp_handle, (ULONG_PTR)&awaitable->accept_socket,
                                   0)) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(socket);
    awaitable->success = false;
  }

  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
}

auto OverlappedTcpAccept::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;
  std::cerr << utils::with_location(std::format("TcpAccept compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
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
  auto init_buf = char{};
  auto bytes_recived = DWORD{};
  auto addr_size = sizeof(sockaddr_in) + 16;
  auto accept_result = ::AcceptEx(impl->socket, accept_socket, &init_buf, 0, addr_size, addr_size, &bytes_recived,
                                  (OVERLAPPED *)&awaitable.impl->ovex);
  if (not accept_result) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("AcceptEx failed: {}", ::WSAGetLastError()))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return awaitable;
    }
  }

  // return awaitable
  awaitable.accept_socket = TcpSocket{ts};
  awaitable.accept_socket.impl->socket = accept_socket;
  awaitable.success = true;
  return awaitable;
}

} // namespace cotask

// TcpConnect
namespace cotask {

auto OverlappedTcpConnect::io_succeed() -> void {
  awaitable->finished = true;
  awaitable->success = true;
  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
}

auto OverlappedTcpConnect::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;
  std::cerr << utils::with_location(std::format("TcpConnect compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
}

TcpConnect::TcpConnect(TaskScheduler &ts) : ts{ts} {
  CONSTRUCT_IMPL(this);
}

TcpConnect::~TcpConnect() {
  std::destroy_at(impl);
}

auto TcpSocket::connect(std::string_view ip, std::string_view port) -> TcpConnect {
  auto awaitable = TcpConnect{ts};

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

  if (not ::CreateIoCompletionPort(std::bit_cast<HANDLE>(impl->socket), ts.impl->iocp_handle, (ULONG_PTR)this, 0)) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(impl->socket);
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

  // connect
  auto connect_result = impl->fnConnectEx(impl->socket, connect_addr_list->ai_addr, sizeof(sockaddr), nullptr, 0,
                                          nullptr, (OVERLAPPED *)&awaitable.impl->ovex);
  if (not connect_result) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("ConnectEx failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));

      ::freeaddrinfo(connect_addr_list);
      return awaitable;
    }
  }

  ::freeaddrinfo(connect_addr_list);

  awaitable.success = true;
  return awaitable;
}

} // namespace cotask

// TcpClose
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

} // namespace cotask

// TcpRecvOnce
namespace cotask {

auto OverlappedTcpRecvOnce::io_succeed(DWORD bytes_recived) -> void {
  awaitable->finished = true;
  awaitable->success = true;
  awaitable->buf = {awaitable->buf.data(), bytes_recived};
  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
}

auto OverlappedTcpRecvOnce::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;
  std::cerr << utils::with_location(std::format("TcpRecvOnce compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
}

TcpRecvOnce::TcpRecvOnce(TaskScheduler &ts, std::span<char> buf) : ts{ts}, buf{buf} {
  CONSTRUCT_IMPL(this);
}

TcpRecvOnce::~TcpRecvOnce() {
  std::destroy_at(impl);
}

auto TcpSocket::recv_once(std::span<char> buf) -> TcpRecvOnce {
  auto awaitable = TcpRecvOnce{ts, buf};

  auto wsa_buf = WSABUF{
    .len = static_cast<ULONG>(buf.size()),
    .buf = buf.data(),
  };
  auto bytes_recived = ULONG{};
  auto flags = ULONG{};

  const auto recv_result =
    ::WSARecv(impl->socket, &wsa_buf, 1ul, &bytes_recived, &flags, (OVERLAPPED *)&awaitable.impl->ovex, nullptr);

  if (recv_result != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("WSARecv failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return awaitable;
    }
  }

  awaitable.success = true;
  return awaitable;
}

} // namespace cotask

// TcpRecvAll
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
  std::cerr << utils::with_location(std::format("TcpRecvAll compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  // TODO
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
  return awaitable;
}

} // namespace cotask

// TcpSendOnce
namespace cotask {

auto OverlappedTcpSendOnce::io_succeed(DWORD bytes_sent) -> void {
  awaitable->finished = true;
  awaitable->success = true;
  awaitable->bytes_sent = bytes_sent;
  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
}

auto OverlappedTcpSendOnce::io_failed(DWORD err_code) -> void {
  awaitable->finished = true;
  awaitable->success = false;
  std::cerr << utils::with_location(std::format("TcpSendOnce compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
  if (awaitable->cohandle) {
    *awaitable->await_subtask = false;
    awaitable->ts.schedule_waiting_done({awaitable->cohandle, awaitable->await_subtask});
  }
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
  auto bytes_sent = ULONG{};

  const auto send_result =
    ::WSASend(impl->socket, &wsa_buf, 1ul, &bytes_sent, 0, (OVERLAPPED *)&awaitable.impl->ovex, nullptr);

  if (send_result != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("WSASend failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return awaitable;
    }
  }

  awaitable.success = true;
  return awaitable;
}

} // namespace cotask

// TcpSendAll
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
  return awaitable;
}

} // namespace cotask
