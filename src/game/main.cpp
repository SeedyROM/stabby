#include <iostream>

#include <engine/engine.h>

int main(int argc, char *argv[]) {
  // Build a window
  auto window = ste::Window::builder()
                    .setTitle("Stabby : v0.0.1")
                    .setSize(1280, 720)
                    .setVSync(true)
                    .build()
                    .value_or(nullptr);

  // Check if window creation failed
  if (!window) {
    std::cerr << "Failed to create window!" << std::endl;
    return -1;
  }

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
      // sceneManager->handleEvent(event);
    }

    // sceneManager->update(timer.getDeltaTime());

    window->clearColor(0.1f, 0.1f, 0.1f, 1.0f);
    // sceneManager->render();
    window->swapBuffers();
  }

  return 0;
}