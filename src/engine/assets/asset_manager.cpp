#include "asset_manager.h"

namespace ste {

std::optional<AssetManager> AssetManager::create(CreateInfo &createInfo) {
  try {
    return AssetManager(createInfo.numThreads);
  } catch (const std::exception &e) {
    createInfo.success = false;
    createInfo.errorMsg = e.what();
    return std::nullopt;
  }
}

AssetManager::AssetManager(size_t numThreads)
    : m_threadPool(std::make_unique<ThreadPool>(numThreads)) {}

AssetManager::~AssetManager() { clear(); }

AssetManager::AssetManager(AssetManager &&other) noexcept
    : m_threadPool(std::move(other.m_threadPool)),
      m_assets(std::move(other.m_assets)),
      m_totalAssets(other.m_totalAssets.load()),
      m_loadedAssets(other.m_loadedAssets.load()) {}

AssetManager &AssetManager::operator=(AssetManager &&other) noexcept {
  if (this != &other) {
    m_threadPool = std::move(other.m_threadPool);

    std::lock_guard<std::mutex> lock(m_assetsMutex);
    std::lock_guard<std::mutex> otherLock(other.m_assetsMutex);

    m_assets = std::move(other.m_assets);
    m_totalAssets = other.m_totalAssets.load();
    m_loadedAssets = other.m_loadedAssets.load();
  }
  return *this;
}

template <typename T>
AssetHandle<T> AssetManager::load(const std::string &path) {
  std::lock_guard<std::mutex> lock(m_assetsMutex);

  auto it = m_assets.find(path);
  if (it != m_assets.end() && it->second.isType<T>()) {
    if (auto asset = std::static_pointer_cast<T>(it->second.asset)) {
      it->second.refCount++;
      return AssetHandle<T>(asset);
    }
  }

  try {
    m_totalAssets++;

    // Load Shader
    if constexpr (std::is_same_v<T, Shader>) {
      Shader::CreateInfo createInfo;
      if (auto shader = Shader::createFromFilesystem(
              path + ".vert", path + ".frag", createInfo)) {
        auto asset = std::make_shared<Shader>(std::move(*shader));
        m_assets.try_emplace(path, asset, 1);
        m_loadedAssets++;
        return AssetHandle<T>(asset);
      } else {
        m_totalAssets--;
        throw std::runtime_error(createInfo.errorMsg);
      }
      // Load Texture
    } else if constexpr (std::is_same_v<T, Texture>) {
      Texture::CreateInfo createInfo;
      if (auto texture = Texture::createFromFile(path, createInfo)) {
        auto asset = std::make_shared<Texture>(std::move(*texture));
        m_assets.try_emplace(path, asset, 1);
        m_loadedAssets++;
        return AssetHandle<T>(asset);
      } else {
        m_totalAssets--;
        throw std::runtime_error(createInfo.errorMsg);
      }
    } else {
      // Asset type-specific loading logic here
      throw std::runtime_error("Unsupported asset type");
    }
  } catch (const std::exception &e) {
    m_totalAssets--;
    throw;
  }
}

template <typename T>
std::future<AssetHandle<T>> AssetManager::loadAsync(const std::string &path) {
  m_totalAssets++;
  return m_threadPool->enqueue([this, path]() -> AssetHandle<T> {
    try {
      return this->load<T>(path);
    } catch (const std::exception &e) {
      m_totalAssets--;
      throw;
    }
  });
}

template <typename T> bool AssetManager::exists(const std::string &path) const {
  std::lock_guard<std::mutex> lock(m_assetsMutex);
  auto it = m_assets.find(path);
  return it != m_assets.end() && it->second.isType<T>();
}

template <typename T> void AssetManager::remove(const std::string &path) {
  std::lock_guard<std::mutex> lock(m_assetsMutex);
  auto it = m_assets.find(path);
  if (it != m_assets.end() && it->second.isType<T>()) {
    if (--it->second.refCount == 0) {
      m_assets.erase(it);
      m_totalAssets--;
      m_loadedAssets--;
    }
  }
}

void AssetManager::clear() {
  std::lock_guard<std::mutex> lock(m_assetsMutex);
  m_assets.clear();
  m_totalAssets = 0;
  m_loadedAssets = 0;
}

float AssetManager::getLoadProgress() const {
  size_t total = m_totalAssets.load();
  return total > 0 ? static_cast<float>(m_loadedAssets.load()) / total : 1.0f;
}

// Explicit template instantiations
template AssetHandle<Shader> AssetManager::load<Shader>(const std::string &);
template std::future<AssetHandle<Shader>>
AssetManager::loadAsync<Shader>(const std::string &);
template bool AssetManager::exists<Shader>(const std::string &) const;
template void AssetManager::remove<Shader>(const std::string &);

template AssetHandle<Texture> AssetManager::load<Texture>(const std::string &);
template std::future<AssetHandle<Texture>>
AssetManager::loadAsync<Texture>(const std::string &);
template bool AssetManager::exists<Texture>(const std::string &) const;
template void AssetManager::remove<Texture>(const std::string &);

} // namespace ste