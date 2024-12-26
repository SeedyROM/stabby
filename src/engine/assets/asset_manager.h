#pragma once

#include "asset_loader.h"
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace ste {

class AssetManager {
private:
  // We split the interface into sync and async parts
  struct SyncLoaderInterface {
    virtual ~SyncLoaderInterface() = default;
    virtual std::any load(AssetLoader &loader, const std::string &path) = 0;
    virtual bool exists(const AssetLoader &loader,
                        const std::string &path) const = 0;
    virtual void remove(AssetLoader &loader, const std::string &path) = 0;
  };

  template <typename T> class TypedLoader : public SyncLoaderInterface {
  public:
    std::any load(AssetLoader &loader, const std::string &path) override {
      return loader.load<T>(path);
    }

    bool exists(const AssetLoader &loader,
                const std::string &path) const override {
      return loader.exists<T>(path);
    }

    void remove(AssetLoader &loader, const std::string &path) override {
      loader.remove<T>(path);
    }

    // Async loading is separate from the interface
    std::future<AssetHandle<T>> loadAsync(AssetLoader &loader,
                                          const std::string &path) {
      return loader.loadAsync<T>(path);
    }
  };

  struct AssetEntry {
    std::string filePath;
    std::type_index type;
    std::any handle;

    AssetEntry() : type(typeid(void)) {}

    AssetEntry(const std::string &path, const std::type_index &t, std::any h)
        : filePath(path), type(t), handle(h) {}
  };

public:
  explicit AssetManager(std::shared_ptr<AssetLoader> loader)
      : m_loader(std::move(loader)) {}

  template <typename T> void registerType() {
    m_loaders[std::type_index(typeid(T))] = std::make_unique<TypedLoader<T>>();
  }

  void registerDefaults() {
    registerType<Shader>();
    registerType<Texture>();
    registerType<AudioFile>();
    registerType<Font>();
  }

  template <typename T>
  AssetHandle<T> load(const std::string &name, const std::string &filePath) {
    auto it = m_loaders.find(std::type_index(typeid(T)));
    if (it == m_loaders.end()) {
      throw std::runtime_error("Asset type not registered: " +
                               std::string(typeid(T).name()));
    }

    auto result = it->second->load(*m_loader, filePath);
    auto handle = std::any_cast<AssetHandle<T>>(result);

    m_assets[name] = AssetEntry(filePath, std::type_index(typeid(T)), handle);
    return handle;
  }

  template <typename T>
  std::future<AssetHandle<T>> loadAsync(const std::string &name,
                                        const std::string &filePath) {
    auto it = m_loaders.find(std::type_index(typeid(T)));
    if (it == m_loaders.end()) {
      throw std::runtime_error("Asset type not registered: " +
                               std::string(typeid(T).name()));
    }

    // Get the typed loader to do async loading
    auto typedLoader = static_cast<TypedLoader<T> *>(it->second.get());
    return typedLoader->loadAsync(*m_loader, filePath);
  }

  template <typename T> AssetHandle<T> get(const std::string &name) {
    auto it = m_assets.find(name);
    if (it == m_assets.end()) {
      throw std::runtime_error("Asset not found: " + name);
    }
    if (it->second.type != std::type_index(typeid(T))) {
      throw std::runtime_error("Asset type mismatch for: " + name);
    }
    return std::any_cast<AssetHandle<T>>(it->second.handle);
  }

  bool exists(const std::string &name) const {
    return m_assets.find(name) != m_assets.end();
  }

  template <typename T> bool exists(const std::string &name) const {
    auto it = m_assets.find(name);
    return it != m_assets.end() &&
           it->second.type == std::type_index(typeid(T));
  }

  void remove(const std::string &name) {
    auto it = m_assets.find(name);
    if (it != m_assets.end()) {
      auto loaderIt = m_loaders.find(it->second.type);
      if (loaderIt != m_loaders.end()) {
        loaderIt->second->remove(*m_loader, it->second.filePath);
      }
      m_assets.erase(it);
    }
  }

  std::vector<std::string> getAssetNames() const {
    std::vector<std::string> names;
    names.reserve(m_assets.size());
    for (const auto &[name, _] : m_assets) {
      names.push_back(name);
    }
    return names;
  }

  float getLoadProgress() const { return m_loader->getLoadProgress(); }

private:
  std::shared_ptr<AssetLoader> m_loader;
  std::unordered_map<std::type_index, std::unique_ptr<SyncLoaderInterface>>
      m_loaders;
  std::unordered_map<std::string, AssetEntry> m_assets;
};

} // namespace ste