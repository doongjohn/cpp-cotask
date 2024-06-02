#include <cstdlib>
#include <array>
#include <iostream>

#include <cotask/tcp.hpp>

auto main() -> int {
  cotask::init();

  auto ts = cotask::TaskScheduler{};

  auto async_fn = [](cotask::TaskScheduler &ts) -> cotask::Task<void> {
    auto listen_socket = cotask::TcpSocket{ts};
    listen_socket.bind(8000);
    listen_socket.listen();

    std::cout << "server accept\n";
    auto accept_result = co_await listen_socket.accept();
    if (accept_result.success) {
      auto &conn_socket = accept_result.accept_socket;

      std::cout << "server send\n";
      auto send_msg = std::string{"hello from tcp server!"};
      co_await conn_socket.send_once(send_msg);

      std::cout << "server recv\n";
      auto recv_buf = std::array<char, 22>{};
      auto recv_result = co_await conn_socket.recv_once(recv_buf);
      std::cout << std::string_view{recv_result.buf.data(), recv_result.buf.size()} << '\n';

      std::cout << "server close\n";
      conn_socket.close();
    }

    listen_socket.close();
    co_return;
  };

  ts.schedule_from_sync(async_fn(ts));
  ts.execute();

  cotask::deinit();
  return EXIT_SUCCESS;
}
