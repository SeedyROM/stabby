#include "scene.h"

#include <stdexcept>

namespace ste {

SceneManager::~SceneManager() { clearScenes(); }

void SceneManager::registerScene(const std::string &name,
                                 SceneFactory factory) {
  m_sceneFactories[name] = std::move(factory);
}

bool SceneManager::hasScene(const std::string &name) const {
  return m_sceneFactories.contains(name);
}

void SceneManager::pushScene(const std::string &name) {
  auto newScene = createScene(name);
  if (!newScene) {
    throw std::runtime_error("Failed to create scene: " + name);
  }

  // Pause current scene if it exists
  if (!m_sceneStack.empty()) {
    m_sceneStack.back()->onPause();
  }

  // Initialize and push new scene
  initScene(newScene);
  newScene->onInit(); // Call onInit after resources are set
  m_sceneStack.push_back(newScene);
  newScene->onEnter();
}

void SceneManager::popScene() {
  if (m_sceneStack.empty()) {
    return;
  }

  // Clean up current scene
  m_sceneStack.back()->onExit();
  m_sceneStack.pop_back();

  // Resume previous scene if it exists
  if (!m_sceneStack.empty()) {
    m_sceneStack.back()->onResume();
  }
}

void SceneManager::setScene(const std::string &name) {
  // Create new scene first to ensure it's valid
  auto newScene = createScene(name);
  if (!newScene) {
    throw std::runtime_error("Failed to create scene: " + name);
  }

  // Clear existing scenes
  clearScenes();

  // Initialize and push new scene
  initScene(newScene);
  newScene->onInit(); // Call onInit after resources are set
  m_sceneStack.push_back(newScene);
  newScene->onEnter();
}

void SceneManager::clearScenes() {
  while (!m_sceneStack.empty()) {
    m_sceneStack.back()->onExit();
    m_sceneStack.pop_back();
  }
}

Scene *SceneManager::getCurrentScene() {
  return m_sceneStack.empty() ? nullptr : m_sceneStack.back().get();
}

const Scene *SceneManager::getCurrentScene() const {
  return m_sceneStack.empty() ? nullptr : m_sceneStack.back().get();
}

World *SceneManager::getCurrentWorld() {
  if (auto scene = getCurrentScene()) {
    return &scene->getWorld();
  }
  return nullptr;
}

const World *SceneManager::getCurrentWorld() const {
  if (auto scene = getCurrentScene()) {
    return &scene->getWorld();
  }
  return nullptr;
}

void SceneManager::update(float deltaTime) {
  if (!m_sceneStack.empty()) {
    m_sceneStack.back()->update(deltaTime);
  }
}

void SceneManager::render() {
  if (!m_sceneStack.empty()) {
    m_sceneStack.back()->render();
  }
}

void SceneManager::handleEvent(const SDL_Event &event) {
  if (!m_sceneStack.empty()) {
    m_sceneStack.back()->handleEvent(event);
  }
}

std::shared_ptr<Scene> SceneManager::createScene(const std::string &name) {
  auto it = m_sceneFactories.find(name);
  if (it == m_sceneFactories.end()) {
    return nullptr;
  }
  return it->second();
}

void SceneManager::initScene(std::shared_ptr<Scene> scene) {
  scene->setAssetLoader(m_assetLoader);
  scene->setRenderer(m_renderer);
  scene->setCamera(m_camera);
  scene->setAudioManager(m_audioManager);
}

} // namespace ste