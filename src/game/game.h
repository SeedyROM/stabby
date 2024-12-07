// game.h
#pragma once

#include "engine/engine.h"
#include <random>

namespace game {

struct Transform {
  glm::vec2 position;
  glm::vec2 scale;
  float rotation;
};

struct Velocity {
  float dx;
  float dy;
};

struct Spinny {
  float rotation;

  Spinny() {
    rotation =
        (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f -
         1.0f) *
        0.8f;
  }
};

struct TimeScaleState {
  float currentScale = 1.0f;
  float targetScale = 1.0f;
  float transitionSpeed = 0.3f;
};

class GameScene : public ste::Scene {
public:
  GameScene() : m_timer(60) {} // Initialize timer with 60 FPS target

  void onInit() override {
    // Initialize scene resources
    m_world.addResource(std::make_shared<TimeScaleState>());
    setupSystems();
    loadAssets();
  }

  void onEnter() override {
    // Start playing music
    if (m_music) {
      m_audioManager->getEngine().playMusic(m_music, true);
    }
  }

  void update(float deltaTime) override {
    // If the scene is paused, don't update
    if (isPaused())
      return;

    // Update input
    m_inputManager.update();
    handleInput();

    // Update the game timer, camera, and world
    m_timer.update();
    updateCamera(m_timer.getDeltaTime());
    m_world.update(m_timer.getDeltaTime());
  }

  void render() override {
    m_renderer->beginScene(m_camera->getViewProjectionMatrix());
    m_world.render();
    m_renderer->endScene();
  }

private:
  // Time scale presets mapped to keys
  const std::unordered_map<ste::Input, float> TIME_SCALE_PRESETS = {
      {ste::Input::Num1, 0.66f},
      {ste::Input::Num2, 0.90f},
      {ste::Input::Num3, 1.00f},
      {ste::Input::Num4, 1.25f},
      {ste::Input::Num5, 1.55f}};

  void handleInput() {
    // Handle time scale changes
    for (const auto &[input, scale] : TIME_SCALE_PRESETS) {
      if (m_inputManager.isKeyPressed(input)) {
        setTimeScale(scale);
      }
    }

    // Handle entity spawning
    if (m_inputManager.isKeyPressed(ste::Input::Space)) {
      spawnEntities();
    }
  }

  void setupSystems() {
    // Time scale system
    m_world.addSystem("TimeScaleUpdate", [this](ste::World &world) {
      auto time = world.getResource<ste::Time>();
      auto timeScale = world.getResource<TimeScaleState>();

      if (timeScale->currentScale != timeScale->targetScale) {
        float delta = timeScale->transitionSpeed * time->deltaSeconds;

        if (timeScale->currentScale < timeScale->targetScale) {
          timeScale->currentScale =
              std::min(timeScale->currentScale + delta, timeScale->targetScale);
        } else {
          timeScale->currentScale =
              std::max(timeScale->currentScale - delta, timeScale->targetScale);
        }

        m_timer.setTimeScale(timeScale->currentScale);
        m_audioManager->getEngine().setSpeed(timeScale->currentScale);
      }
    });

    // Physics system
    m_world.addSystem("Physics", [](ste::World &world) {
      auto time = world.getResource<ste::Time>();
      ste::Query<Transform, Velocity, Spinny> query(&world);

      for (auto [entity, transform, vel, spinny] : query) {
        transform.position.x += vel.dx * time->deltaSeconds;
        transform.position.y += vel.dy * time->deltaSeconds;
        transform.rotation += spinny.rotation * time->deltaSeconds;

        // Wrap around screen edges
        if (transform.position.x < 0)
          transform.position.x = 1280;
        if (transform.position.x > 1280)
          transform.position.x = 0;
        if (transform.position.y < 0)
          transform.position.y = 720;
        if (transform.position.y > 720)
          transform.position.y = 0;
      }
    });

    // Rendering system
    m_world.addSystem(
        "Rendering",
        [this](ste::World &world) {
          if (!m_textureHandle)
            return;

          ste::Renderer2D::TextureInfo texInfo{m_textureHandle->getId(),
                                               m_textureHandle->getWidth(),
                                               m_textureHandle->getHeight()};

          ste::Query<Transform, Spinny> query(&world);
          for (auto [entity, transform, spinny] : query) {
            m_renderer->drawTexturedQuad(
                {transform.position.x, 720 - transform.position.y}, texInfo,
                transform.scale, {1.0f, 1.0f, 1.0f, 1.0f}, transform.rotation,
                {0.0f, 0.0f, 1.0f, 1.0f});
          }
        },
        0, true);
  }

  void loadAssets() {
    m_textureHandle = m_assetLoader->load<ste::Texture>(
        ste::getAssetPath("textures/albert.png"));
    m_click =
        m_assetLoader->load<ste::AudioFile>(ste::getAssetPath("sfx/click.wav"));
    m_slowDown = m_assetLoader->load<ste::AudioFile>(
        ste::getAssetPath("sfx/slowdown.wav"));
    m_music = m_assetLoader->load<ste::AudioFile>(
        ste::getAssetPath("music/level.ogg"));
  }

  void setTimeScale(float scale) {
    auto timeScale = m_world.getResource<TimeScaleState>();
    if (timeScale->currentScale != scale) {
      timeScale->targetScale = scale;
      m_audioManager->getEngine().setSpeed(scale);
      if (m_slowDown) {
        m_audioManager->getEngine().playSound(m_slowDown);
      }
    }
  }

  void spawnEntities() {
    if (m_click) {
      m_audioManager->getEngine().playSound(m_click);
    }

    for (int i = 0; i < 50; i++) {
      float size = 64.0f + (rand() % 192);
      m_world.spawn()
          .with(Transform{{static_cast<float>(rand() % 1280),
                           static_cast<float>(rand() % 720)},
                          {size, size},
                          0.0f})
          .with(Velocity{static_cast<float>(rand() % 100 - 50),
                         static_cast<float>(rand() % 100 - 50)})
          .with(Spinny{});
    }
  }

  void updateCamera(float deltaTime) {
    const float CAMERA_SPEED = 500.0f;

    // Get input state
    glm::vec2 moveDir(0.0f);
    if (m_inputManager.isKeyHeld(ste::Input::W))
      moveDir.y += 1.0f;
    if (m_inputManager.isKeyHeld(ste::Input::S))
      moveDir.y -= 1.0f;
    if (m_inputManager.isKeyHeld(ste::Input::A))
      moveDir.x -= 1.0f;
    if (m_inputManager.isKeyHeld(ste::Input::D))
      moveDir.x += 1.0f;

    // Normalize diagonal movement
    if (glm::length(moveDir) > 0.0f) {
      moveDir = glm::normalize(moveDir);
    }

    // Update zoom
    if (m_inputManager.isKeyHeld(ste::Input::Q)) {
      m_camera->addZoom(-0.1f * deltaTime);
    }
    if (m_inputManager.isKeyHeld(ste::Input::E)) {
      m_camera->addZoom(0.1f * deltaTime);
    }

    // Update camera movement
    m_camera->addVelocity(moveDir * CAMERA_SPEED * deltaTime);
    m_camera->update(deltaTime);
  }

  // Asset handles
  ste::AssetHandle<ste::Texture> m_textureHandle;
  ste::AssetHandle<ste::AudioFile> m_click;
  ste::AssetHandle<ste::AudioFile> m_slowDown;
  ste::AssetHandle<ste::AudioFile> m_music;

  // Input manager
  ste::InputManager m_inputManager;

  // Game timer
  ste::GameTimer m_timer;
};

} // namespace game