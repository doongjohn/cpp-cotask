#include "timer.hpp"

#include <cotask/impl.hpp>

namespace cotask {

Timer::Timer(std::uint64_t timeout) : timeout{timeout} {
  IMPL_CONSTRUCT();

  auto due_time = ULARGE_INTEGER{.QuadPart = -(timeout * 10000)};
  impl->due_time.dwHighDateTime = due_time.HighPart;
  impl->due_time.dwLowDateTime = due_time.LowPart;
}

Timer::~Timer() {
  close();
  std::destroy_at(impl);
}

auto Timer::start() -> void {
  ended = false;
  if (impl->timer != nullptr) {
    ::SetThreadpoolTimerEx(impl->timer, nullptr, 0, 0);
    ::WaitForThreadpoolTimerCallbacks(impl->timer, TRUE);
    ::SetThreadpoolTimerEx(impl->timer, &impl->due_time, 0, 0);
  }
}

auto Timer::close() -> void {
  // https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpooltimer#remarks
  if (impl->timer != nullptr) {
    ::SetThreadpoolTimerEx(impl->timer, nullptr, 0, 0);
    ::WaitForThreadpoolTimerCallbacks(impl->timer, TRUE);
    ::CloseThreadpoolTimer(impl->timer);
    impl->timer = nullptr;
  }
}

} // namespace cotask
