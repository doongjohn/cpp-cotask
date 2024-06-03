#include <cstdlib>
#include <array>
#include <iostream>

#include <cotask/file.hpp>

[[nodiscard]] auto async_fn0(cotask::TaskScheduler &) -> cotask::Task<void> {
  for (auto i = 1; i <= 10; ++i) {
    std::cout << "async_fn0 - step " << i << '\n';
    co_await std::suspend_always{};
  }
  std::cout << "async_fn0 - done\n";
  co_return;
};

[[nodiscard]] auto async_fn1(cotask::TaskScheduler &ts) -> cotask::Task<void> {
  std::cout << "async_fn1 - start\n";
  auto buf = std::array<char, 14>{};
  auto read_buf = cotask::FileReadBuf{ts, "src/cotask/utils.hpp", buf};
  auto read_all = cotask::FileReadAll{ts, "src/cotask/utils.hpp"};
  auto read_buf_result = co_await read_buf;
  auto read_all_result = co_await read_all;
  std::cout << "async_fn1 - read_buf_result:\n" << read_buf_result.to_string_view() << '\n';
  std::cout << "async_fn1 - read_all_result:\n" << read_all_result.to_string_view() << '\n';
  std::cout << "async_fn1 - done\n";
  co_return;
};

[[nodiscard]] auto async_fn2(cotask::TaskScheduler &ts) -> cotask::Task<void> {
  std::cout << "async_fn2 - start\n";
  for (auto i = 0; i < 2; ++i) {
    auto buf = std::array<char, 14>{};
    auto read_buf = cotask::FileReadBuf{ts, "src/cotask/utils.hpp", buf};
    co_await async_fn1(ts);
    auto read_all = cotask::FileReadAll{ts, "src/cotask/utils.hpp"};
    auto read_buf_result = co_await read_buf;
    auto read_all_result = co_await read_all;
    std::cout << "async_fn2 - read_buf_result:\n" << read_buf_result.to_string_view() << '\n';
    std::cout << "async_fn2 - read_all_result:\n" << read_all_result.to_string_view() << '\n';
  }
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
