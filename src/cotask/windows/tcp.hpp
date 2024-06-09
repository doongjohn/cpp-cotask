#pragma once

#include <cotask/tcp.hpp>

#include <bit>

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>

// TcpSocket
namespace cotask {

struct TcpSocket::Impl {
  SOCKET socket = INVALID_SOCKET;
  LPFN_CONNECTEX fnConnectEx = nullptr;
  // TODO: DisconnectEx

  inline Impl() = default;
  inline Impl(const Impl &other) = default;

  inline auto operator=(const Impl &other) -> Impl & {
    if (this == &other) {
      return *this;
    }
    this->socket = other.socket;
    this->fnConnectEx = other.fnConnectEx;
    return *this;
  }

  inline auto get_handle() -> HANDLE {
    return std::bit_cast<HANDLE>(socket);
  }
};

} // namespace cotask

// Accept
namespace cotask {

struct OverlappedTcpAccept : public OVERLAPPED {
  const TcpIoType type = TcpIoType::Accept;
  TcpAccept *awaitable;

  inline explicit OverlappedTcpAccept(TcpAccept *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_received(DWORD bytes_received) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpAccept::Impl {
  OverlappedTcpAccept ovex;

  alignas(8) std::uint8_t addr_buf[88]{};
  DWORD bytes_received = 0;

  inline explicit Impl(TcpAccept *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// Connect
namespace cotask {

struct OverlappedTcpConnect : public OVERLAPPED {
  const TcpIoType type = TcpIoType::Connect;
  TcpConnect *awaitable;

  inline explicit OverlappedTcpConnect(TcpConnect *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_received(DWORD bytes_received) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpConnect::Impl {
  OverlappedTcpConnect ovex;

  inline explicit Impl(TcpConnect *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// Recv
namespace cotask {

struct OverlappedTcpRecv : public OVERLAPPED {
  const TcpIoType type = TcpIoType::Recv;
  TcpRecv *awaitable;

  inline explicit OverlappedTcpRecv(TcpRecv *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_received(DWORD bytes_received) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpRecv::Impl {
  OverlappedTcpRecv ovex;

  inline explicit Impl(TcpRecv *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// RecvAll
namespace cotask {

struct OverlappedTcpRecvAll : public OVERLAPPED {
  const TcpIoType type = TcpIoType::RecvAll;
  TcpRecvAll *awaitable;

  inline explicit OverlappedTcpRecvAll(TcpRecvAll *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_request() -> bool;
  auto io_received(DWORD bytes_received) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpRecvAll::Impl {
  OverlappedTcpRecvAll ovex;

  inline explicit Impl(TcpRecvAll *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// Send
namespace cotask {

struct OverlappedTcpSend : public OVERLAPPED {
  const TcpIoType type = TcpIoType::Send;
  TcpSend *awaitable;

  inline explicit OverlappedTcpSend(TcpSend *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_sent(DWORD bytes_sent) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpSend::Impl {
  OverlappedTcpSend ovex;

  inline explicit Impl(TcpSend *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// SendAll
namespace cotask {

struct OverlappedTcpSendAll : public OVERLAPPED {
  const TcpIoType type = TcpIoType::SendAll;
  TcpSendAll *awaitable;

  inline explicit OverlappedTcpSendAll(TcpSendAll *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_request() -> bool;
  auto io_sent(DWORD bytes_sent) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpSendAll::Impl {
  OverlappedTcpSendAll ovex;

  inline explicit Impl(TcpSendAll *awaitable) : ovex{awaitable} {}
};

} // namespace cotask
