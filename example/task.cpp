#include <cstdlib>
#include <iostream>

#include <cotask/cotask.hpp>

[[nodiscard]] auto async_nested_task1(cotask::TaskScheduler &) -> cotask::Task<void> {
  std::cout << "nested_task1 - step 1\n";
  co_await std::suspend_always{};
  std::cout << "nested_task1 - step 2\n";
  co_await std::suspend_always{};
  std::cout << "nested_task1 - done\n";
}

[[nodiscard]] auto async_nested_task2(cotask::TaskScheduler &) -> cotask::Task<void> {
  std::cout << "nested_task2 - step 1\n";
  co_await std::suspend_always{};
  std::cout << "nested_task2 - done\n";
}

[[nodiscard]] auto async_nested_task3(cotask::TaskScheduler &, std::string input) -> cotask::Task<std::string> {
  std::cout << "nested_task3 - step 1\n";
  co_await std::suspend_always{};
  std::cout << "nested_task3 - step 2\n";
  co_await std::suspend_always{};
  std::cout << "nested_task3 - done\n";

  co_return input;
}

[[nodiscard]] auto async_outer_task(cotask::TaskScheduler &ts) -> cotask::Task<void> {
  std::cout << "outer_task - step 1\n";

  auto t1 = async_nested_task1(ts);
  auto t2 = async_nested_task2(ts);
  auto t3 = async_nested_task3(ts, "hello coroutine!");

  co_await t1;
  co_await t2;
  auto t3_result = co_await t3;
  std::cout << "t3 result: " << t3_result << '\n';

  std::cout << "outer_task - step 2\n";
  co_await std::suspend_always{};
  std::cout << "outer_task - done\n";
}

[[nodiscard]] auto async_task1(cotask::TaskScheduler &) -> cotask::Task<void> {
  std::cout << "task1 - step 1\n";
  co_await std::suspend_always{};
  std::cout << "task1 - step 2\n";
  co_await std::suspend_always{};
  std::cout << "task1 - done\n";
}

[[nodiscard]] auto async_task2(cotask::TaskScheduler &) -> cotask::Task<void> {
  std::cout << "task2 - step 1\n";
  co_await std::suspend_always{};
  std::cout << "task2 - step 2\n";
  co_await std::suspend_always{};
  std::cout << "task2 - done\n";
}

auto main() -> int {
  auto ts = cotask::TaskScheduler{};
  ts.schedule_from_sync(async_outer_task(ts));
  ts.schedule_from_sync(async_task1(ts));
  ts.schedule_from_sync(async_task2(ts));
  ts.execute();

  return EXIT_SUCCESS;
}
