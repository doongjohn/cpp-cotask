#include <cstdlib>
#include <array>
#include <format>
#include <iostream>

#include <cotask/tcp.hpp>

auto async_server(cotask::TaskScheduler &ts, cotask::TcpSocket listen_socket, int n) -> cotask::Task<void> {
  std::cout << std::format("server {} - accpet\n", n);
  auto client_socket = cotask::TcpSocket{ts};
  auto accept_result = co_await cotask::TcpAccept{&listen_socket, &client_socket};
  if (not accept_result.success) {
    co_return;
  }

  while (true) {
    std::cout << std::format("server {} - send\n", n);
    auto send_buf = std::string{"hello from tcp server!"};
    auto send_result = co_await cotask::TcpSendOnce{&client_socket, send_buf};
    if (not send_result.success) {
      break;
    }

    std::cout << std::format("server {} - recv\n", n);
    auto recv_buf = std::array<char, 22>{};
    auto recv_result = co_await cotask::TcpRecvOnce{&client_socket, recv_buf};
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
  cotask::net_init();

  auto ts = cotask::TaskScheduler{};
  auto listen_socket = cotask::TcpSocket{ts};
  listen_socket.listen(8000);

  for (auto i = 0; i < 2; ++i) {
    ts.schedule_from_sync(async_server(ts, listen_socket, i));
  }
  std::cout << "server execute\n";
  ts.execute();

  listen_socket.close();

  cotask::net_deinit();
  return EXIT_SUCCESS;
}
