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

  auto io_succeed() -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpAccept::Impl {
  OverlappedTcpAccept ovex;

  inline explicit Impl(TcpAccept *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// Connect
namespace cotask {

struct OverlappedTcpConnect : public OVERLAPPED {
  const TcpIoType type = TcpIoType::Connect;

  TcpConnect *awaitable;

  inline explicit OverlappedTcpConnect(TcpConnect *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_succeed() -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpConnect::Impl {
  OverlappedTcpConnect ovex;

  inline explicit Impl(TcpConnect *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// RecvOnce
namespace cotask {

struct OverlappedTcpRecvOnce : public OVERLAPPED {
  const TcpIoType type = TcpIoType::RecvOnce;

  TcpRecvOnce *awaitable;

  inline explicit OverlappedTcpRecvOnce(TcpRecvOnce *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_succeed(DWORD bytes_recived) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpRecvOnce::Impl {
  OverlappedTcpRecvOnce ovex;

  inline explicit Impl(TcpRecvOnce *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// RecvAll
namespace cotask {

struct OverlappedTcpRecvAll : public OVERLAPPED {
  const TcpIoType type = TcpIoType::RecvAll;

  TcpRecvAll *awaitable;

  inline explicit OverlappedTcpRecvAll(TcpRecvAll *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_recived(DWORD bytes_transferred) -> void;
  auto io_request() -> bool;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpRecvAll::Impl {
  OverlappedTcpRecvAll ovex;

  inline explicit Impl(TcpRecvAll *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// SendOnce
namespace cotask {

struct OverlappedTcpSendOnce : public OVERLAPPED {
  const TcpIoType type = TcpIoType::SendOnce;

  TcpSendOnce *awaitable;

  inline explicit OverlappedTcpSendOnce(TcpSendOnce *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_succeed(DWORD bytes_sent) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpSendOnce::Impl {
  OverlappedTcpSendOnce ovex;

  inline explicit Impl(TcpSendOnce *awaitable) : ovex{awaitable} {}
};

} // namespace cotask

// SendAll
namespace cotask {

struct OverlappedTcpSendAll : public OVERLAPPED {
  const TcpIoType type = TcpIoType::SendAll;

  TcpSendAll *awaitable;

  inline explicit OverlappedTcpSendAll(TcpSendAll *awaitable) : OVERLAPPED{}, awaitable{awaitable} {}

  auto io_sent(DWORD bytes_transferred) -> void;
  auto io_request() -> bool;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpSendAll::Impl {
  OverlappedTcpSendAll ovex;

  inline explicit Impl(TcpSendAll *awaitable) : ovex{awaitable} {}
};

} // namespace cotask
