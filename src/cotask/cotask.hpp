#pragma once

#include <cassert>
#include <coroutine>
#include <vector>
#include <deque>

namespace cotask {

auto init() -> void;
auto deinit() -> void;

struct ScheduledTask {
  std::coroutine_handle<> cohandle;
  bool *await_subtask = nullptr;

  inline ScheduledTask(std::coroutine_handle<> cohandle, bool *await_subtask)
      : cohandle{cohandle}, await_subtask{await_subtask} {}

  [[nodiscard]] inline auto done() const noexcept -> bool {
    return cohandle.done();
  }

  [[nodiscard]] inline auto can_resume() const noexcept -> bool {
    return not cohandle.done() and (await_subtask == nullptr or not *await_subtask);
  }

  inline auto resume() const -> void {
    cohandle.resume();
  }
};

template <typename T>
struct Task;

struct TaskScheduler {
  template <typename T>
  friend struct Task;

public:
  struct Impl;
  uint8_t impl_storage[40]{};
  Impl *impl;

private:
  std::size_t waiting_task_count = 0;
  std::deque<ScheduledTask> async_tasks;
  std::vector<std::coroutine_handle<>> ended_task;
  std::vector<std::coroutine_handle<>> from_sync_tasks;

public:
  TaskScheduler();
  TaskScheduler(const TaskScheduler &other) = delete;
  ~TaskScheduler();

public:
  inline auto operator==(const TaskScheduler &other) const -> bool {
    return this == std::addressof(other);
  }

public:
  template <typename T>
  inline auto schedule_from_sync(Task<T> &&task) -> void;

  inline auto schedule_from_task(ScheduledTask task) -> void {
    async_tasks.push_back(task);
  }

  inline auto add_waiting_task_count() -> void {
    waiting_task_count += 1;
  }

  inline auto schedule_waiting_done(ScheduledTask task) -> void {
    assert(waiting_task_count != 0);
    assert(task.await_subtask != nullptr);

    waiting_task_count -= 1;
    *task.await_subtask = false;
    async_tasks.push_back(task);
  }

  auto execute() -> void;

private:
  inline auto destroy_cohandle(std::coroutine_handle<> cohandle) -> void {
    ended_task.push_back(cohandle);
  }
};

struct TaskPromise {
  TaskScheduler &ts;
  std::coroutine_handle<> parent_cohandle = nullptr;
  bool *parent_await_subtask = nullptr;
  bool await_subtask = false;

  inline explicit TaskPromise(TaskScheduler &ts) : ts(ts) {}
};

template <>
struct Task<void> {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type : public TaskPromise {
    template <typename... Args>
    promise_type(TaskScheduler &ts, Args...) : TaskPromise{ts} {}

    inline auto get_return_object() -> Task {
      return Task{coro_handle::from_promise(*this)};
    }
    inline auto initial_suspend() noexcept -> std::suspend_always {
      return {};
    }
    inline auto final_suspend() noexcept -> std::suspend_always {
      if (parent_await_subtask != nullptr) {
        ts.schedule_waiting_done({parent_cohandle, parent_await_subtask});
      }
      return {};
    }
    inline auto return_void() -> void {}
    inline auto unhandled_exception() -> void {
      std::terminate();
    }
  };

  coro_handle cohandle;
  promise_type &promise;

  inline explicit Task(const coro_handle cohandle) : cohandle{cohandle}, promise{cohandle.promise()} {
    promise.ts.schedule_from_task({cohandle, &promise.await_subtask});
  }

  inline Task(const Task &) = delete;

  inline Task(Task &&other) noexcept : cohandle{other.cohandle}, promise{other.promise} {
    promise.parent_cohandle = other.promise.parent_cohandle;
    promise.parent_await_subtask = other.promise.parent_await_subtask;
    promise.await_subtask = other.promise.await_subtask;
    other.cohandle = nullptr;
    other.promise.parent_cohandle = nullptr;
    other.promise.parent_await_subtask = nullptr;
  }

  [[nodiscard]] inline auto await_ready() const noexcept -> bool {
    return cohandle.done();
  }

  template <typename TaskT>
  inline auto await_suspend(std::coroutine_handle<Task::promise_type> parent_cohandle) const noexcept -> void {
    promise.parent_cohandle = parent_cohandle;
    promise.parent_await_subtask = &parent_cohandle.promise().await_subtask;
    *promise.parent_await_subtask = true;
    promise.ts.add_waiting_task_count();
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> parent_cohandle) const noexcept -> void {
    promise.parent_cohandle = parent_cohandle;
    promise.parent_await_subtask = &parent_cohandle.promise().await_subtask;
    *promise.parent_await_subtask = true;
    promise.ts.add_waiting_task_count();
  }

  inline auto await_resume() const noexcept -> void {
    promise.ts.destroy_cohandle(cohandle);
  }
};

template <typename T>
struct Task {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type : public TaskPromise {
    T result;

    template <typename... Args>
    promise_type(TaskScheduler &ts, Args...) : TaskPromise{ts} {}

    inline auto get_return_object() -> Task {
      return Task{coro_handle::from_promise(*this)};
    }
    inline auto initial_suspend() noexcept -> std::suspend_always {
      return {};
    }
    inline auto final_suspend() noexcept -> std::suspend_always {
      if (parent_await_subtask != nullptr) {
        ts.schedule_waiting_done({parent_cohandle, parent_await_subtask});
      }
      return {};
    }
    inline auto return_value(T value) -> void {
      result = value;
    }
    inline auto unhandled_exception() -> void {
      std::terminate();
    }
  };

  coro_handle cohandle;
  promise_type &promise;

  inline explicit Task(const coro_handle cohandle) : cohandle{cohandle}, promise{cohandle.promise()} {
    promise.ts.schedule_from_task({cohandle, &promise.await_subtask});
  }

  inline Task(const Task &) = delete;

  inline Task(Task &&other) noexcept : cohandle{other.cohandle}, promise{other.promise} {
    promise.parent_cohandle = other.promise.parent_cohandle;
    promise.parent_await_subtask = other.promise.parent_await_subtask;
    promise.await_subtask = other.promise.await_subtask;
    promise.result = other.promise.result;
    other.cohandle = nullptr;
    other.promise.parent_cohandle = nullptr;
    other.promise.parent_await_subtask = nullptr;
  }

  [[nodiscard]] inline auto await_ready() const noexcept -> bool {
    return cohandle.done();
  }

  template <typename TaskT>
  inline auto await_suspend(std::coroutine_handle<Task::promise_type> parent_cohandle) const noexcept -> void {
    promise.parent_cohandle = parent_cohandle;
    promise.parent_await_subtask = &parent_cohandle.promise().await_subtask;
    *promise.parent_await_subtask = true;
    promise.ts.add_waiting_task_count();
  }

  inline auto await_suspend(std::coroutine_handle<Task<void>::promise_type> parent_cohandle) const noexcept -> void {
    promise.parent_cohandle = parent_cohandle;
    promise.parent_await_subtask = &parent_cohandle.promise().await_subtask;
    *promise.parent_await_subtask = true;
    promise.ts.add_waiting_task_count();
  }

  [[nodiscard]] inline auto await_resume() const noexcept -> T {
    promise.ts.destroy_cohandle(cohandle);
    return std::move(promise.result);
  }
};

template <typename T>
inline auto TaskScheduler::schedule_from_sync(Task<T> &&task) -> void {
  from_sync_tasks.push_back(task.cohandle);
}

} // namespace cotask
