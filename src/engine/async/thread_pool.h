// thread_pool.h
#pragma once

#include <cinttypes>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace ste {

class ThreadPool {
public:
  explicit ThreadPool(size_t numThreads);
  ~ThreadPool();

  // Delete copy operations
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  // Move operations
  ThreadPool(ThreadPool &&other) noexcept;
  ThreadPool &operator=(ThreadPool &&other) noexcept;

  template <typename F, typename... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<typename std::invoke_result_t<F, Args...>> {
    using return_type = typename std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      if (m_stop) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }

      m_tasks.emplace([task]() { (*task)(); });
    }
    m_condition.notify_one();
    return res;
  }

private:
  void stop();

  std::vector<std::thread> m_workers;
  std::queue<std::function<void()>> m_tasks;

  std::mutex m_queueMutex;
  std::condition_variable m_condition;
  bool m_stop;
};

} // namespace ste