#include "cotask.hpp"
#include "tcp.hpp"

#include <cotask/impl.hpp>
#include <cotask/utils.hpp>

#include <iostream>

#include <ws2tcpip.h>

// TcpSocket
namespace cotask {

TcpSocket::TcpSocket(TaskScheduler &ts) : ts{ts} {
  IMPL_CONSTRUCT();
}

TcpSocket::TcpSocket(const TcpSocket &other) : ts{other.ts} {
  IMPL_COPY(*other.impl);
}

TcpSocket::~TcpSocket() {
  std::destroy_at(impl);
}

auto TcpSocket::operator=(const TcpSocket &other) -> TcpSocket & {
  if (this == &other) {
    return *this;
  }
  *this->impl = *other.impl;
  this->ts = other.ts;
  return *this;
}

} // namespace cotask

// Listen
namespace cotask {

auto TcpSocket::listen(std::uint16_t port) -> bool {
  // create socket
  impl->socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  if (impl->socket == INVALID_SOCKET) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSASocketW failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return false;
  }

  // bind
  auto addr = SOCKADDR_IN{};
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
  addr.sin_port = ::htons(port);
  if (::bind(impl->socket, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("socket bind failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(impl->socket);
    return false;
  }

  // enable SO_CONDITIONAL_ACCEPT
  auto on = TRUE;
  if (::setsockopt(impl->socket, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (const char *)&on, sizeof(on)) != 0) {
    ::closesocket(impl->socket);
    return false;
  }

  // setup IOCP
  if (not ::CreateIoCompletionPort(impl->get_handle(), ts.impl->iocp_handle, (ULONG_PTR)this, 0)) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(impl->socket);
    return false;
  }

  // listen
  if (::listen(impl->socket, SOMAXCONN) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("listen failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(impl->socket);
    return false;
  }

  return true;
}

} // namespace cotask

// Accept
namespace cotask {

TcpAcceptResult::TcpAcceptResult(bool finished, bool success, TcpSocket *accept_socket)
    : finished{finished}, success{success} {
  if (finished and success) {
    // setup iocp
    if (not ::CreateIoCompletionPort(accept_socket->impl->get_handle(), accept_socket->ts.impl->iocp_handle,
                                     (ULONG_PTR)accept_socket, 0)) {
      const auto err_code = ::GetLastError();
      std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      ::closesocket(accept_socket->impl->socket);
      success = false;
    }
  }
}

auto OverlappedTcpAccept::io_received(DWORD) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = true;
}

auto OverlappedTcpAccept::io_failed(DWORD err_code) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = false;

  std::cerr << utils::with_location(std::format("TcpAccept compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpAccept::TcpAccept(TcpSocket *sock, TcpSocket *accept_socket)
    : tcp_socket{*sock}, ts{sock->ts}, accept_socket{accept_socket} {
  IMPL_CONSTRUCT(this);

  // create socket
  auto conn_socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  if (conn_socket == INVALID_SOCKET) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSASocketW failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return;
  }

  // accept
  auto accept_success = ::AcceptEx(tcp_socket.impl->socket, conn_socket, impl->addr_buf, 0, sizeof(impl->addr_buf) / 2,
                                   sizeof(impl->addr_buf) / 2, &impl->bytes_received, &impl->ovex);
  if (not accept_success) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("AcceptEx failed: {}", ::WSAGetLastError()))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      ::closesocket(conn_socket);
      return;
    }
  }

  success = true;
  accept_socket->impl->socket = conn_socket;
}

TcpAccept::~TcpAccept() {
  std::destroy_at(impl);
}

} // namespace cotask

// Connect
namespace cotask {

auto OverlappedTcpConnect::io_received(DWORD) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = true;
}

auto OverlappedTcpConnect::io_failed(DWORD err_code) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = false;

  std::cerr << utils::with_location(std::format("TcpConnect compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpConnect::TcpConnect(TcpSocket *sock, std::string_view ip, std::string_view port) : tcp_socket{*sock}, ts{sock->ts} {
  IMPL_CONSTRUCT(this);

  // create socket
  tcp_socket.impl->socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  if (tcp_socket.impl->socket == INVALID_SOCKET) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("WSASocketW failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    return;
  }

  // get ConnectEx function pointer
  if (tcp_socket.impl->fnConnectEx == nullptr) {
    auto guid = GUID WSAID_CONNECTEX;
    auto bytes = DWORD{};
    auto fn_load_result =
      ::WSAIoctl(tcp_socket.impl->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),
                 &tcp_socket.impl->fnConnectEx, sizeof(tcp_socket.impl->fnConnectEx), &bytes, nullptr, nullptr);
    if (fn_load_result != 0) {
      const auto err_code = ::WSAGetLastError();
      std::cerr << utils::with_location(std::format("ConnectEx load failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      ::closesocket(tcp_socket.impl->socket);
      return;
    }
  }

  // bind
  auto addr = SOCKADDR_IN{};
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
  addr.sin_port = ::htons(0);
  if (::bind(tcp_socket.impl->socket, (SOCKADDR *)&addr, sizeof(addr)) != 0) {
    const auto err_code = ::WSAGetLastError();
    std::cerr << utils::with_location(std::format("socket bind failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(tcp_socket.impl->socket);
    return;
  }

  // setup IOCP
  if (not ::CreateIoCompletionPort(tcp_socket.impl->get_handle(), ts.impl->iocp_handle, (ULONG_PTR)sock, 0)) {
    const auto err_code = ::GetLastError();
    std::cerr << utils::with_location(std::format("CreateIoCompletionPort failed: {}", err_code))
              << std::format("err msg: {}\n", std::system_category().message((int)err_code));
    ::closesocket(tcp_socket.impl->socket);
    return;
  }

  // resolve the server address and port
  auto connect_addr_list = PADDRINFOA{};
  auto addr_hints = addrinfo{};
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;

  auto getaddr_result = ::getaddrinfo(ip.data(), port.data(), &addr_hints, &connect_addr_list);
  if (getaddr_result != 0) {
    std::cerr << utils::with_location(std::format("getaddrinfo failed: {}\n", getaddr_result))
              << std::format("err msg: {}\n", ::gai_strerrorA(getaddr_result));
    ::closesocket(tcp_socket.impl->socket);
    return;
  }

  // connect
  auto connect_success = tcp_socket.impl->fnConnectEx(tcp_socket.impl->socket, connect_addr_list->ai_addr,
                                                      sizeof(SOCKADDR), nullptr, 0, nullptr, &impl->ovex);
  ::freeaddrinfo(connect_addr_list);
  if (not connect_success) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("ConnectEx failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      ::closesocket(tcp_socket.impl->socket);
      return;
    }
  }

  success = true;
}

TcpConnect::~TcpConnect() {
  std::destroy_at(impl);
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

  auto linger = LINGER{
    .l_onoff = 0,
    .l_linger = 0,
  };
  if (::setsockopt(impl->socket, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger)) != 0) {
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

// Recv
namespace cotask {

auto OverlappedTcpRecv ::io_received(DWORD bytes_received) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = true;
  awaitable->bytes_received = bytes_received;
  awaitable->buf = {awaitable->buf.data(), bytes_received};
}

auto OverlappedTcpRecv ::io_failed(DWORD err_code) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = false;

  std::cerr << utils::with_location(std::format("TcpRecv  compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpRecv::TcpRecv(TcpSocket *sock, std::span<char> buf) : tcp_socket{*sock}, ts{sock->ts}, buf{buf} {
  IMPL_CONSTRUCT(this);

  auto wsa_buf = WSABUF{
    .len = static_cast<ULONG>(buf.size()),
    .buf = buf.data(),
  };
  auto flags = ULONG{};

  // recv
  auto recv_result =
    ::WSARecv(tcp_socket.impl->socket, &wsa_buf, 1ul, (PULONG)&bytes_received, &flags, &impl->ovex, nullptr);
  if (recv_result != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("WSARecv failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return;
    }
  }

  success = true;
}

TcpRecv::~TcpRecv() {
  std::destroy_at(impl);
}

} // namespace cotask

// RecvAll
namespace cotask {

auto OverlappedTcpRecvAll::io_request() -> bool {
  // TODO
  return true;
}

auto OverlappedTcpRecvAll::io_received(DWORD bytes_received) -> void {
  // TOOD
}

auto OverlappedTcpRecvAll::io_failed(DWORD err_code) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = false;

  std::cerr << utils::with_location(std::format("TcpRecvAll compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpRecvAll::TcpRecvAll(TcpSocket *sock, std::span<char> buf) : tcp_socket{*sock}, ts{sock->ts}, buf{buf} {
  IMPL_CONSTRUCT(this);
  // TODO

  success = true;
}

TcpRecvAll::~TcpRecvAll() {
  std::destroy_at(impl);
}

} // namespace cotask

// Send
namespace cotask {

auto OverlappedTcpSend ::io_sent(DWORD bytes_sent) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = true;
  awaitable->bytes_sent = bytes_sent;
}

auto OverlappedTcpSend ::io_failed(DWORD err_code) -> void {
  if (awaitable->is_waiting != nullptr) {
    *awaitable->is_waiting = false;
  }
  awaitable->finished = true;
  awaitable->success = false;

  std::cerr << utils::with_location(std::format("TcpSend  compeletion failed: {}", err_code))
            << std::format("err msg: {}\n", std::system_category().message((int)err_code));
}

TcpSend::TcpSend(TcpSocket *sock, std::span<const char> buf) : tcp_socket{*sock}, ts{sock->ts}, buf{buf} {
  IMPL_CONSTRUCT(this);

  auto wsa_buf = WSABUF{
    .len = static_cast<ULONG>(buf.size()),
    .buf = const_cast<char *>(buf.data()),
  };

  // send
  auto send_result = ::WSASend(tcp_socket.impl->socket, &wsa_buf, 1ul, (PULONG)&bytes_sent, 0, &impl->ovex, nullptr);
  if (send_result != 0) {
    const auto err_code = ::WSAGetLastError();
    if (err_code != WSA_IO_PENDING) {
      std::cerr << utils::with_location(std::format("WSASend failed: {}", err_code))
                << std::format("err msg: {}\n", std::system_category().message((int)err_code));
      return;
    }
  }

  success = true;
}

TcpSend::~TcpSend() {
  std::destroy_at(impl);
}

} // namespace cotask

// SendAll
namespace cotask {

auto OverlappedTcpSendAll::io_request() -> bool {
  // TODO
  return true;
}

auto OverlappedTcpSendAll::io_sent(DWORD bytes_sent) -> void {
  // TODO
}

auto OverlappedTcpSendAll::io_failed(DWORD err_code) -> void {
  // TODO
}

TcpSendAll::TcpSendAll(TcpSocket *sock, std::span<char> buf) : tcp_socket{*sock}, ts{sock->ts}, buf{buf} {
  IMPL_CONSTRUCT(this);
  // TODO

  success = true;
}

TcpSendAll::~TcpSendAll() {
  std::destroy_at(impl);
}

} // namespace cotask
