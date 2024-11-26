#pragma once

#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "engine/async/thread_pool.h"
#include "engine/rendering/shader.h"

namespace ste {

template <typename T> class AssetHandle {
public:
  AssetHandle() = default;
  explicit AssetHandle(std::shared_ptr<T> asset) : m_asset(asset) {}

  T *operator->() { return m_asset.get(); }
  const T *operator->() const { return m_asset.get(); }
  T &operator*() { return *m_asset; }
  const T &operator*() const { return *m_asset; }

  bool isValid() const { return m_asset != nullptr; }
  operator bool() const { return isValid(); }

private:
  std::shared_ptr<T> m_asset;
};

class AssetManager {
public:
  struct CreateInfo {
    std::string errorMsg;
    bool success = true;
    size_t numThreads = std::thread::hardware_concurrency();
  };

  static std::optional<AssetManager> create(CreateInfo &createInfo);

  ~AssetManager();
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;
  AssetManager(AssetManager &&other) noexcept;
  AssetManager &operator=(AssetManager &&other) noexcept;

  // Synchronous loading
  template <typename T> AssetHandle<T> load(const std::string &path);

  // Asynchronous loading
  template <typename T>
  std::future<AssetHandle<T>> loadAsync(const std::string &path);

  // Check if asset exists
  template <typename T> bool exists(const std::string &path) const;

  // Remove asset
  template <typename T> void remove(const std::string &path);

  // Clear all assets
  void clear();

  // Get asset load progress (0.0f - 1.0f)
  float getLoadProgress() const;

private:
  explicit AssetManager(size_t numThreads);

  struct AssetEntry {
    std::shared_ptr<void> asset;
    size_t refCount = 0;
    const void *typeId; // Store raw type_info address

    template <typename T>
    AssetEntry(std::shared_ptr<T> a, size_t count)
        : asset(a), refCount(count), typeId(&typeid(T)) {}

    template <typename T> bool isType() const { return typeId == &typeid(T); }
  };

  std::unique_ptr<ThreadPool> m_threadPool;
  std::unordered_map<std::string, AssetEntry> m_assets;
  mutable std::mutex m_assetsMutex;
  std::atomic<size_t> m_totalAssets{0};
  std::atomic<size_t> m_loadedAssets{0};
};

} // namespace ste