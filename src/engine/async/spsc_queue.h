#pragma once

#include <array>
#include <atomic>
#include <concepts>
#include <optional>

namespace ste {

// Generic SPSC Queue with customizable size and alignment
template <typename T, size_t Size = 1024>
requires std::default_initializable<T> && std::copyable<T>
class SPSCQueue {
  // Ensure proper cache line alignment to prevent false sharing
  alignas(64) std::atomic<size_t> writeIndex{0};
  alignas(64) std::atomic<size_t> readIndex{0};
  alignas(64) std::array<T, Size> buffer;

public:
  // Returns the capacity of the queue
  static constexpr size_t capacity() { return Size; }

  // Returns current number of items in the queue
  [[nodiscard]] size_t size() const {
    size_t write = writeIndex.load(std::memory_order_relaxed);
    size_t read = readIndex.load(std::memory_order_relaxed);
    return write >= read ? write - read : Size - (read - write);
  }

  // Returns true if the queue is empty
  [[nodiscard]] bool empty() const {
    return readIndex.load(std::memory_order_relaxed) ==
           writeIndex.load(std::memory_order_acquire);
  }

  // Returns true if the queue is full
  [[nodiscard]] bool full() const {
    size_t next = (writeIndex.load(std::memory_order_relaxed) + 1) % Size;
    return next == readIndex.load(std::memory_order_acquire);
  }

  // Try to push an item to the queue
  template <typename U = T>
  requires std::convertible_to<U, T>
  bool try_push(U &&item) {
    size_t current = writeIndex.load(std::memory_order_relaxed);
    size_t next = (current + 1) % Size;

    if (next == readIndex.load(std::memory_order_acquire)) {
      return false; // Queue is full
    }

    buffer[current] = std::forward<U>(item);
    writeIndex.store(next, std::memory_order_release);
    return true;
  }

  // Try to pop an item from the queue
  std::optional<T> try_pop() {
    size_t current = readIndex.load(std::memory_order_relaxed);

    if (current == writeIndex.load(std::memory_order_acquire)) {
      return std::nullopt; // Queue is empty
    }

    T item = std::move(buffer[current]);
    readIndex.store((current + 1) % Size, std::memory_order_release);
    return item;
  }

  // Clear the queue
  void clear() {
    while (!empty()) {
      (void)try_pop();
    }
  }
};

}; // namespace ste