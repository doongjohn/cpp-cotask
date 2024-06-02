#include <cstdlib>
#include <array>
#include <iostream>

#include <cotask/tcp.hpp>

auto main() -> int {
  cotask::init();

  auto ts = cotask::TaskScheduler{};

  auto async_fn = [](cotask::TaskScheduler &ts) -> cotask::Task<void> {
    auto conn_socket = cotask::TcpSocket{ts};

    std::cout << "client connect\n";
    auto connect_result = co_await conn_socket.connect("localhost", "8000");
    if (connect_result.success) {
      std::cout << "client recv\n";
      auto recv_buf = std::array<char, 22>{};
      auto recv_result = co_await conn_socket.recv_once(recv_buf);
      std::cout << std::string_view{recv_result.buf.data(), recv_result.buf.size()} << '\n';

      std::cout << "client send\n";
      auto send_msg = std::string{"hello from tcp client!"};
      co_await conn_socket.send_once(send_msg);

      std::cout << "client close\n";
      conn_socket.close();
    }

    co_return;
  };

  ts.schedule_from_sync(async_fn(ts));
  ts.execute();

  cotask::deinit();
  return EXIT_SUCCESS;
}
