#include <iostream>

#include <engine/engine.h>

#include "game.h"

int main(int argc, char *argv[]) {
  // Create window
  auto window = ste::Window::builder()
                    .setTitle("Stabby : v0.0.1")
                    .setSize(1280, 720)
                    .setVSync(true)
                    .build()
                    .value_or(nullptr);

  if (!window) {
    std::cerr << "Failed to create window!" << std::endl;
    return -1;
  }

  // Create core systems
  ste::AudioManager audio;
  ste::Renderer2D::CreateInfo rendererCreateInfo;
  auto renderer = ste::Renderer2D::create(rendererCreateInfo);
  if (!renderer) {
    std::cerr << "Failed to create renderer!" << std::endl;
    return -1;
  }
  ste::AssetLoader::CreateInfo assetCreateInfo;
  auto assetLoader = ste::AssetLoader::create(assetCreateInfo);
  if (!assetLoader) {
    std::cerr << "Failed to create asset loader!" << std::endl;
    return -1;
  }

  auto camera =
      std::make_shared<ste::Camera2D>(window->getWidth(), window->getHeight());

  // Create and setup scene manager
  ste::SceneManager sceneManager;
  sceneManager.setRenderer(
      std::make_shared<ste::Renderer2D>(std::move(*renderer)));
  sceneManager.setAssetLoader(
      std::make_shared<ste::AssetLoader>(std::move(*assetLoader)));
  sceneManager.setCamera(camera);
  sceneManager.setAudioManager(
      std::make_shared<ste::AudioManager>(std::move(audio)));

  // Register game scene
  sceneManager.registerScene(
      "game", []() { return std::make_shared<game::GameScene>(); });

  // Start with game scene
  sceneManager.pushScene("game");

  // Initialize game timer
  ste::GameTimer timer(60);

  // Main game loop
  bool running = true;
  while (running) {
    timer.update();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
      sceneManager.handleEvent(event);
    }

    sceneManager.update(timer.getDeltaTime());

    window->clearColor(0.2f, 0.2f, 0.2f, 1.0f);
    sceneManager.render();
    window->swapBuffers();

    timer.limitFrameRate();
  }

  return 0;
}