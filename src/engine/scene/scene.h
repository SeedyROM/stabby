#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/assets/asset_loader.h"
#include "engine/audio/audio_manager.h"
#include "engine/rendering/camera_2d.h"
#include "engine/rendering/renderer_2d.h"
#include "engine/world/world.h"

namespace ste {

class Scene {
public:
  Scene() = default;
  virtual ~Scene() = default;

  // Delete copy operations
  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;

  // Core lifecycle methods
  virtual void onInit() {} // Called when resources are set, before onEnter
  virtual void onEnter() {}
  virtual void onExit() {}
  virtual void onPause() {}
  virtual void onResume() {}
  virtual void update(float deltaTime) {}
  virtual void render() {}
  virtual void handleEvent(const SDL_Event &event) {}

  // Resource getters
  World &getWorld() { return m_world; }
  const World &getWorld() const { return m_world; }
  std::shared_ptr<AssetLoader> getAssetLoader() { return m_assetLoader; }
  std::shared_ptr<Renderer2D> getRenderer() { return m_renderer; }
  std::shared_ptr<Camera2D> getCamera() { return m_camera; }
  std::shared_ptr<AudioManager> getAudioManager() { return m_audioManager; }

  // Scene state
  bool isPaused() const { return m_paused; }
  void setPaused(bool paused) { m_paused = paused; }

  // Resource setters
  void setAssetLoader(std::shared_ptr<AssetLoader> manager) {
    m_assetLoader = manager;
  }
  void setRenderer(std::shared_ptr<Renderer2D> renderer) {
    m_renderer = renderer;
  }
  void setCamera(std::shared_ptr<Camera2D> camera) { m_camera = camera; }
  void setAudioManager(std::shared_ptr<AudioManager> audio) {
    m_audioManager = audio;
  }

protected:
  std::shared_ptr<AssetLoader> m_assetLoader;
  std::shared_ptr<Renderer2D> m_renderer;
  std::shared_ptr<Camera2D> m_camera;
  std::shared_ptr<AudioManager> m_audioManager;
  World m_world;
  bool m_paused = false;
};

class SceneManager {
public:
  using SceneFactory = std::function<std::shared_ptr<Scene>()>;

  SceneManager() = default;
  ~SceneManager();

  // Delete copy operations
  SceneManager(const SceneManager &) = delete;
  SceneManager &operator=(const SceneManager &) = delete;

  // Scene registration
  void registerScene(const std::string &name, SceneFactory factory);
  bool hasScene(const std::string &name) const;

  // Scene stack operations
  void pushScene(const std::string &name);
  void popScene();
  void setScene(const std::string &name);
  void clearScenes();

  // Scene access
  Scene *getCurrentScene();
  const Scene *getCurrentScene() const;
  size_t getSceneCount() const { return m_sceneStack.size(); }

  // Core update loop
  void update(float deltaTime);
  void render();
  void handleEvent(const SDL_Event &event);

  // Resource management
  void setAssetLoader(std::shared_ptr<AssetLoader> manager) {
    m_assetLoader = manager;
  }
  void setRenderer(std::shared_ptr<Renderer2D> renderer) {
    m_renderer = renderer;
  }
  void setCamera(std::shared_ptr<Camera2D> camera) { m_camera = camera; }
  void setAudioManager(std::shared_ptr<AudioManager> audio) {
    m_audioManager = audio;
  }

  // Get the world of the current scene
  World *getCurrentWorld();
  const World *getCurrentWorld() const;

private:
  std::shared_ptr<Scene> createScene(const std::string &name);
  void initScene(std::shared_ptr<Scene> scene);

  std::vector<std::shared_ptr<Scene>> m_sceneStack;
  std::unordered_map<std::string, SceneFactory> m_sceneFactories;

  std::shared_ptr<AssetLoader> m_assetLoader;
  std::shared_ptr<Renderer2D> m_renderer;
  std::shared_ptr<Camera2D> m_camera;
  std::shared_ptr<AudioManager> m_audioManager;
};

} // namespace ste