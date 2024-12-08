#include <iostream>

#include <glm/glm.hpp>

#include <engine/engine.h>

int main(int argc, char *argv[]) {

  auto window = ste::Window::builder()
                    .setTitle("Stabby Editor : v0.0.1")
                    .setSize(1280, 720)
                    .setVSync(true)
                    .build()
                    .value_or(nullptr);
  if (!window) {
    std::cerr << "Failed to create window!" << std::endl;
    return -1;
  }

  ste::AssetLoader::CreateInfo assetCreateInfo;
  auto assetLoader = ste::AssetLoader::create(assetCreateInfo);
  if (!assetLoader) {
    std::cerr << "Failed to create asset loader!" << std::endl;
    return -1;
  }

  auto font = assetLoader->load<ste::Font>("assets/fonts/better-vcr.ttf@16");

  ste::Renderer2D::CreateInfo rendererCreateInfo;
  auto renderer = ste::Renderer2D::create(rendererCreateInfo);
  if (!renderer) {
    std::cerr << "Failed to create renderer!" << std::endl;
    return -1;
  }

  ste::TextRenderer textRenderer(*renderer);
  ste::AudioManager audioManager;
  ste::InputManager inputManager;
  ste::Camera2D camera(window->getWidth(), window->getHeight());
  ste::GameTimer timer(120);
  ste::World world;

  glm::vec2 gridSize = {64.0f, 64.0f};
  glm::vec2 cursorPosition = {0.0f, 0.0f};

  bool running = true;
  while (running) {
    // Update the game timer
    timer.update();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    inputManager.update();

    window->clearColor(0.361f, 0.361f, 0.471f, 1.0f);

    renderer->beginScene(camera.getViewProjectionMatrix());

    // Get the mouse position
    glm::vec2 mousePosition = inputManager.getMousePosition();

    // Get the world space cursor position
    cursorPosition = camera.screenToWorld({mousePosition.x, mousePosition.y});

    // Lock the cursor to a grid
    cursorPosition.x = std::floor(cursorPosition.x / gridSize.x) * gridSize.x;
    cursorPosition.y = std::floor(cursorPosition.y / gridSize.y) * gridSize.y;

    // Draw the a font
    std::string fps = "FPS: " + std::to_string(timer.getFPS());
    textRenderer.renderText(*font, fps, {12.0f, window->getHeight() - 28.0f},
                            {1.0f, 1.0f, 1.0f, 1.0f});

    // Draw a white quad
    renderer->drawQuad({cursorPosition.x + (gridSize.x / 2.0f),
                        cursorPosition.y + (gridSize.y / 2.0f), 0.0f},
                       {gridSize.x, gridSize.y}, {1.0f, 1.0f, 1.0f, 0.0f}, 0.0f,
                       0.1f, {1.0f, 1.0f, 1.0f, 1.0f});

    renderer->endScene();
    window->swapBuffers();

    // Cap the frame rate
    timer.limitFrameRate();
  }

  return 0;
}