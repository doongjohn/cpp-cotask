#include <cstdlib>
#include <array>
#include <string>
#include <format>
#include <iostream>

#include <cotask/tcp.hpp>

[[nodiscard]] auto async_fn(cotask::TaskScheduler &, cotask::TcpSocket &listen_socket) -> cotask::Task<void> {
  static auto n = 0;
  ++n;

  std::cout << std::format("server {} - accpet\n", n);
  auto accept_result = co_await listen_socket.accept();
  if (not accept_result.success) {
    co_return;
  }

  auto &conn_socket = accept_result.accept_socket;
  while (true) {
    std::cout << std::format("server {} - send\n", n);
    auto send_msg = std::string{"hello from tcp server!"};
    auto send_result = co_await conn_socket.send_once(send_msg);
    if (not send_result.success) {
      break;
    }

    std::cout << std::format("server {} - recv\n", n);
    auto recv_buf = std::array<char, 22>{};
    auto recv_result = co_await conn_socket.recv_once(recv_buf);
    if (not recv_result.success) {
      break;
    }
    std::cout << recv_result.to_string_view() << '\n';

    co_await std::suspend_always{};
  }

  std::cout << std::format("server {} - close\n", n);
  conn_socket.close();
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
    ts.schedule_from_sync(async_fn(ts, listen_socket));
  }
  ts.execute();
  listen_socket.close();

  cotask::deinit();
  return EXIT_SUCCESS;
}
