#include <cstdlib>
#include <array>
#include <chrono>
#include <iostream>

#include <cotask/tcp.hpp>

[[nodiscard]] auto async_fn(cotask::TaskScheduler &ts) -> cotask::Task<void> {
  auto socket = cotask::TcpSocket{ts};

  std::cout << "client - connect\n";
  auto connect_result = co_await socket.connect("localhost", "8000");
  if (connect_result.success) {
    const auto timer = 10;
    auto start = std::chrono::high_resolution_clock::now();
    while (true) {
      std::cout << "client - recv\n";
      auto recv_buf = std::array<char, 22>{};
      auto recv_result = co_await socket.recv_once(recv_buf);
      if (not recv_result.success) {
        break;
      }
      std::cout << recv_result.to_string_view() << '\n';

      std::cout << "client - send\n";
      auto send_msg = std::string{"hello from tcp client!"};
      auto send_result = co_await socket.send_once(send_msg);
      if (not send_result.success) {
        break;
      }

      // timer
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
      if (timer <= duration.count()) {
        break;
      }
    }

    std::cout << "client - close\n";
    socket.close();
  }

  co_return;
};

auto main() -> int {
  cotask::init();

  auto ts = cotask::TaskScheduler{};
  ts.schedule_from_sync(async_fn(ts));
  ts.execute();

  cotask::deinit();
  return EXIT_SUCCESS;
}
