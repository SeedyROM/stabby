#include "asset_loader.h"

namespace ste {

std::shared_ptr<AssetLoader> AssetLoader::create(CreateInfo &createInfo) {
  try {
    return std::make_shared<AssetLoader>(createInfo.numThreads);
  } catch (const std::exception &e) {
    createInfo.success = false;
    createInfo.errorMsg = e.what();
    return nullptr;
  }
}

AssetLoader::AssetLoader(size_t numThreads)
    : m_threadPool(std::make_unique<ThreadPool>(numThreads)) {}

AssetLoader::~AssetLoader() { clear(); }

AssetLoader::AssetLoader(AssetLoader &&other) noexcept
    : m_threadPool(std::move(other.m_threadPool)),
      m_assets(std::move(other.m_assets)),
      m_totalAssets(other.m_totalAssets.load()),
      m_loadedAssets(other.m_loadedAssets.load()) {}

AssetLoader &AssetLoader::operator=(AssetLoader &&other) noexcept {
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
AssetHandle<T> AssetLoader::load(const std::string &path) {
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
      // Load AudioFile
    } else if constexpr (std::is_same_v<T, AudioFile>) {
      AudioFile::CreateInfo createInfo;
      if (auto audioFile = AudioFile::createFromFile(path, createInfo)) {
        auto asset = std::make_shared<AudioFile>(std::move(*audioFile));
        m_assets.try_emplace(path, asset, 1);
        m_loadedAssets++;
        return AssetHandle<T>(asset);
      } else {
        m_totalAssets--;
        throw std::runtime_error(createInfo.errorMsg);
      }
    } else if constexpr (std::is_same_v<T, Font>) {
      Font::CreateInfo createInfo;

      // Get the font size from the path
      size_t sizePos = path.find('@');
      if (sizePos != std::string::npos) {
        createInfo.size = std::stoi(path.substr(sizePos + 1));
      }

      // Strip the font size from the path
      std::string actualPath =
          (sizePos != std::string::npos) ? path.substr(0, sizePos) : path;

      if (auto font = Font::createFromFile(actualPath, createInfo)) {
        auto asset = std::make_shared<Font>(std::move(*font));
        m_assets.try_emplace(path, asset, 1);
        m_loadedAssets++;
        return AssetHandle<T>(asset);
      } else {
        m_totalAssets--;
        throw std::runtime_error(createInfo.errorMsg);
      }
    } else if constexpr (std::is_same_v<T, Map>) {
      try {
        auto map = MapSerializer::deserializeFromFile(path);
        return AssetHandle<T>(std::make_shared<Map>(std::move(map)));
      } catch (const std::exception &e) {
        m_totalAssets--;
        throw std::runtime_error("Failed to load map: " +
                                 std::string(e.what()));
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
std::future<AssetHandle<T>> AssetLoader::loadAsync(const std::string &path) {
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

template <typename T> bool AssetLoader::exists(const std::string &path) const {
  std::lock_guard<std::mutex> lock(m_assetsMutex);
  auto it = m_assets.find(path);
  return it != m_assets.end() && it->second.isType<T>();
}

template <typename T> void AssetLoader::remove(const std::string &path) {
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

void AssetLoader::clear() {
  std::lock_guard<std::mutex> lock(m_assetsMutex);
  m_assets.clear();
  m_totalAssets = 0;
  m_loadedAssets = 0;
}

float AssetLoader::getLoadProgress() const {
  size_t total = m_totalAssets.load();
  return total > 0 ? static_cast<float>(m_loadedAssets.load()) / total : 1.0f;
}

// Explicit template instantiations
template AssetHandle<Shader> AssetLoader::load<Shader>(const std::string &);
template std::future<AssetHandle<Shader>>
AssetLoader::loadAsync<Shader>(const std::string &);
template bool AssetLoader::exists<Shader>(const std::string &) const;
template void AssetLoader::remove<Shader>(const std::string &);

template AssetHandle<Texture> AssetLoader::load<Texture>(const std::string &);
template std::future<AssetHandle<Texture>>
AssetLoader::loadAsync<Texture>(const std::string &);
template bool AssetLoader::exists<Texture>(const std::string &) const;
template void AssetLoader::remove<Texture>(const std::string &);

template AssetHandle<AudioFile>
AssetLoader::load<AudioFile>(const std::string &);
template std::future<AssetHandle<AudioFile>>
AssetLoader::loadAsync<AudioFile>(const std::string &);
template bool AssetLoader::exists<AudioFile>(const std::string &) const;
template void AssetLoader::remove<AudioFile>(const std::string &);

template AssetHandle<Font> AssetLoader::load<Font>(const std::string &);
template std::future<AssetHandle<Font>>
AssetLoader::loadAsync<Font>(const std::string &);
template bool AssetLoader::exists<Font>(const std::string &) const;
template void AssetLoader::remove<Font>(const std::string &);

template AssetHandle<Map> AssetLoader::load<Map>(const std::string &);
template std::future<AssetHandle<Map>>
AssetLoader::loadAsync<Map>(const std::string &);
template bool AssetLoader::exists<Map>(const std::string &) const;
template void AssetLoader::remove<Map>(const std::string &);

} // namespace ste