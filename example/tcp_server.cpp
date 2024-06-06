#include <cstdlib>
#include <array>
#include <string>
#include <format>
#include <iostream>

#include <cotask/tcp.hpp>

[[nodiscard]] auto async_fn(cotask::TaskScheduler &, cotask::TcpSocket &listen_socket, int n) -> cotask::Task<void> {
  std::cout << std::format("server {} - accpet\n", n);
  auto accept_result = co_await cotask::TcpAccept{&listen_socket};
  if (not accept_result.success) {
    co_return;
  }

  auto conn_socket = &accept_result.accept_socket;
  while (true) {
    std::cout << std::format("server {} - send\n", n);
    auto send_buf = std::string{"hello from tcp server!"};
    auto send_result = co_await cotask::TcpSendOnce{conn_socket, send_buf};
    if (not send_result.success) {
      break;
    }

    std::cout << std::format("server {} - recv\n", n);
    auto recv_buf = std::array<char, 22>{};
    auto recv_result = co_await cotask::TcpRecvOnce{conn_socket, recv_buf};
    if (not recv_result.success) {
      break;
    }
    std::cout << recv_result.get_string_view() << '\n';
  }

  std::cout << std::format("server {} - close\n", n);
  conn_socket->close();
  co_return;
};

auto main() -> int {
  cotask::init();

  auto ts = cotask::TaskScheduler{};
  auto listen_socket = cotask::TcpSocket{ts};
  listen_socket.bind(8000);
  listen_socket.listen();

  auto max_clients = 3;
  for (auto i = 0; i < max_clients; ++i) {
    ts.schedule_from_sync(async_fn(ts, listen_socket, i));
  }
  ts.execute();
  listen_socket.close();

  cotask::deinit();
  return EXIT_SUCCESS;
}
