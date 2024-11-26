#include "thread_pool.h"

namespace ste {

ThreadPool::ThreadPool(size_t numThreads) : m_stop(false) {
  for (size_t i = 0; i < numThreads; ++i) {
    m_workers.emplace_back([this] {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(m_queueMutex);
          m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });

          if (m_stop && m_tasks.empty()) {
            return;
          }

          task = std::move(m_tasks.front());
          m_tasks.pop();
        }
        task();
      }
    });
  }
}

ThreadPool::~ThreadPool() { stop(); }

void ThreadPool::stop() {
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_stop = true;
  }
  m_condition.notify_all();

  for (std::thread &worker : m_workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  m_workers.clear();
}

ThreadPool::ThreadPool(ThreadPool &&other) noexcept
    : m_workers(std::move(other.m_workers)), m_tasks(std::move(other.m_tasks)),
      m_stop(other.m_stop) {
  other.m_stop = true;
}

ThreadPool &ThreadPool::operator=(ThreadPool &&other) noexcept {
  if (this != &other) {
    stop();

    m_workers = std::move(other.m_workers);
    m_tasks = std::move(other.m_tasks);
    m_stop = other.m_stop;

    other.m_stop = true;
  }
  return *this;
}

} // namespace ste