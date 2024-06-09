#include <cstdlib>
#include <array>
#include <chrono>
#include <iostream>

#include <cotask/tcp.hpp>

auto async_client(cotask::TaskScheduler &ts) -> cotask::Task<void> {
  std::cout << "client - connect\n";
  auto conn_socket = cotask::TcpSocket{ts};
  auto connect_result = co_await cotask::TcpConnect{&conn_socket, "localhost", "8000"};
  if (not connect_result.success) {
    co_return;
  }

  const auto timer = 10;
  auto start = std::chrono::steady_clock::now();

  while (true) {
    std::cout << "client - recv\n";
    auto recv_buf = std::array<char, 22>{};
    auto recv_result = co_await cotask::TcpRecv{&conn_socket, recv_buf};
    if (not recv_result.success) {
      break;
    }
    std::cout << recv_result.get_string_view() << '\n';

    std::cout << "client - send\n";
    auto send_buf = std::string{"hello from tcp client!"};
    auto send_result = co_await cotask::TcpSend{&conn_socket, send_buf};
    if (not send_result.success) {
      break;
    }

    // timer
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    if (timer <= duration.count()) {
      break;
    }
  }

  std::cout << "client - close\n";
  conn_socket.close();
  co_return;
};

auto main() -> int {
  cotask::net_init();

  auto ts = cotask::TaskScheduler{};
  ts.schedule_from_sync(async_client(ts));
  ts.execute();

  cotask::net_deinit();
  return EXIT_SUCCESS;
}
