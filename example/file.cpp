#include <cstdlib>
#include <array>
#include <format>
#include <iostream>

#include <cotask/file.hpp>

auto async_fn0(cotask::TaskScheduler &) -> cotask::Task<void> {
  std::cout << "async_fn0 - start\n";
  for (auto i = 1; i <= 10; ++i) {
    std::cout << std::format("async_fn0 - step {}\n", i);
    co_await std::suspend_always{};
  }
  std::cout << "async_fn0 - done\n";
  co_return;
};

auto async_fn1(cotask::TaskScheduler &ts) -> cotask::Task<void> {
  std::cout << "async_fn1 - start\n";

  auto reader = cotask::FileReader{ts, "src/cotask/utils.hpp"};
  auto buf = std::array<char, 14>{};
  auto read_buf = reader.read_buf(buf);
  auto read_all = reader.read_all();
  auto read_buf_result = co_await read_buf;
  auto read_all_result = co_await read_all;
  std::cout << std::format("async_fn1 - read_buf_result:\n{}\n", read_buf_result.get_string_view());
  std::cout << std::format("async_fn1 - read_all_result:\n{}\n", read_all_result.get_string_view());
  reader.close();

  std::cout << "async_fn1 - done\n";
  co_return;
};

auto async_fn2(cotask::TaskScheduler &ts) -> cotask::Task<void> {
  std::cout << "async_fn2 - start\n";

  auto reader = cotask::FileReader{ts, "src/cotask/utils.hpp"};
  for (auto i = 0; i < 2; ++i) {
    auto buf = std::array<char, 14>{};
    auto read_buf = reader.read_buf(buf);
    co_await async_fn1(ts);
    auto read_all = reader.read_all();
    auto read_buf_result = co_await read_buf;
    auto read_all_result = co_await read_all;
    std::cout << std::format("async_fn2 - read_buf_result:\n{}\n", read_buf_result.get_string_view());
    std::cout << std::format("async_fn2 - read_all_result:\n{}\n", read_all_result.get_string_view());
  }
  reader.close();

  std::cout << "async_fn2 - done\n";
  co_return;
};

auto main() -> int {
  auto ts = cotask::TaskScheduler{};
  ts.schedule_from_sync(async_fn0(ts));
  ts.schedule_from_sync(async_fn2(ts));
  ts.execute();

  return EXIT_SUCCESS;
}
