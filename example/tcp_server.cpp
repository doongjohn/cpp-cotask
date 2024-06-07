#include <cstdlib>
#include <array>
#include <format>
#include <iostream>

#include <cotask/tcp.hpp>

[[nodiscard]] auto async_server(cotask::TaskScheduler &ts, cotask::TcpSocket listen_socket,
                                int n) -> cotask::Task<void> {
  std::cout << std::format("server {} - accpet\n", n);
  auto client_socket = cotask::TcpSocket{ts};
  auto async_accept = cotask::TcpAccept{&listen_socket, &client_socket};
  auto accept_result = co_await async_accept;
  if (not accept_result.success) {
    co_return;
  }
  std::cout << std::format("server {} - post accept\n", n);

  while (true) {
    std::cout << std::format("server {} - send\n", n);
    auto send_buf = std::string{"hello from tcp server!"};
    auto async_send = cotask::TcpSendOnce{&client_socket, send_buf};
    auto send_result = co_await async_send;
    if (not send_result.success) {
      break;
    }

    std::cout << std::format("server {} - recv\n", n);
    auto recv_buf = std::array<char, 22>{};
    auto async_recv = cotask::TcpRecvOnce{&client_socket, recv_buf};
    auto recv_result = co_await async_recv;
    if (not recv_result.success) {
      break;
    }
    std::cout << recv_result.get_string_view() << '\n';

    co_await std::suspend_always{};
  }

  std::cout << std::format("server {} - close\n", n);
  client_socket.close();
  co_return;
};

auto main() -> int {
  cotask::init();

  auto ts = cotask::TaskScheduler{};
  auto listen_socket = cotask::TcpSocket{ts};
  listen_socket.listen(8000);

  auto max_clients = 2;
  for (auto i = 0; i < max_clients; ++i) {
    ts.schedule_from_sync(async_server(ts, listen_socket, i));
  }
  ts.execute();

  listen_socket.close();

  cotask::deinit();
  return EXIT_SUCCESS;
}
