#pragma once

#include "cotask.hpp"
#include <cotask/tcp.hpp>

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>

namespace cotask {

// TcpSocket
struct TcpSocket::Impl {
  const AsyncIoType type = AsyncIoType::TcpSocket;

  SOCKET socket = INVALID_SOCKET;
  LPFN_CONNECTEX fnConnectEx = nullptr;
  // TODO: DisconnectEx

  inline Impl() = default;
  inline Impl(const Impl &other) : socket{other.socket}, fnConnectEx{other.fnConnectEx} {}

  inline auto operator=(const Impl &other) -> Impl & {
    if (this == &other) {
      return *this;
    }
    this->socket = other.socket;
    this->fnConnectEx = other.fnConnectEx;
    return *this;
  }
};

// TcpAccept
struct OverlappedTcpAccept {
  OVERLAPPED ov{};
  const TcpIoType type = TcpIoType::Accept;

  TcpAccept *awaitable;

  inline OverlappedTcpAccept(TcpAccept *awaitable) : awaitable{awaitable} {}

  auto io_succeed() -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpAccept::Impl {
  OverlappedTcpAccept ovex; // OVERLAPPED extened

  inline explicit Impl(TcpAccept *awaitable) : ovex{awaitable} {}
};

// TcpConnect
struct OverlappedTcpConnect {
  OVERLAPPED ov{};
  const TcpIoType type = TcpIoType::Connect;

  TcpConnect *awaitable;

  inline OverlappedTcpConnect(TcpConnect *awaitable) : awaitable{awaitable} {}

  auto io_succeed() -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpConnect::Impl {
  OverlappedTcpConnect ovex; // OVERLAPPED extened

  inline explicit Impl(TcpConnect *awaitable) : ovex{awaitable} {}
};

// TcpRecvOnce
struct OverlappedTcpRecvOnce {
  OVERLAPPED ov{};
  const TcpIoType type = TcpIoType::RecvOnce;

  TcpRecvOnce *awaitable;

  inline OverlappedTcpRecvOnce(TcpRecvOnce *awaitable) : awaitable{awaitable} {}

  auto io_succeed(DWORD bytes_recived) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpRecvOnce::Impl {
  OverlappedTcpRecvOnce ovex; // OVERLAPPED extened

  inline explicit Impl(TcpRecvOnce *awaitable) : ovex{awaitable} {}
};

// TcpRecvAll
struct OverlappedTcpRecvAll {
  OVERLAPPED ov{};
  const TcpIoType type = TcpIoType::RecvAll;

  TcpRecvAll *awaitable;

  inline OverlappedTcpRecvAll(TcpRecvAll *awaitable) : awaitable{awaitable} {}

  auto io_recived(DWORD bytes_transferred) -> void;
  auto io_request() -> bool;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpRecvAll::Impl {
  OverlappedTcpRecvAll ovex; // OVERLAPPED extened

  inline explicit Impl(TcpRecvAll *awaitable) : ovex{awaitable} {}
};

// TcpSendOnce
struct OverlappedTcpSendOnce {
  OVERLAPPED ov{};
  const TcpIoType type = TcpIoType::SendOnce;

  TcpSendOnce *awaitable;

  inline OverlappedTcpSendOnce(TcpSendOnce *awaitable) : awaitable{awaitable} {}

  auto io_succeed(DWORD bytes_sent) -> void;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpSendOnce::Impl {
  OverlappedTcpSendOnce ovex; // OVERLAPPED extened

  inline explicit Impl(TcpSendOnce *awaitable) : ovex{awaitable} {}
};

// TcpSendAll
struct OverlappedTcpSendAll {
  OVERLAPPED ov{};
  const TcpIoType type = TcpIoType::SendAll;

  TcpSendAll *awaitable;

  inline OverlappedTcpSendAll(TcpSendAll *awaitable) : awaitable{awaitable} {}

  auto io_sent(DWORD bytes_transferred) -> void;
  auto io_request() -> bool;
  auto io_failed(DWORD err_code) -> void;
};

struct TcpSendAll::Impl {
  OverlappedTcpSendAll ovex; // OVERLAPPED extened

  inline explicit Impl(TcpSendAll *awaitable) : ovex{awaitable} {}
};

} // namespace cotask
