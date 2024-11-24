#include <iostream>

#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <engine/engine.h>

struct Position {
  float x, y;
};

struct Velocity {
  float dx, dy;
};

int main(int argc, char *argv[]) {
  auto window = ste::Window::create("OpenGL Window Example", 800, 600)
                    .value_or(nullptr); // The value_or is an interesting
                                        // approach to the optionals

  if (!window) {
    std::cerr << "Failed to create window!" << std::endl;
    return -1;
  }

  // Set VSync
  window->setVSync(true);

  // Create a world
  ste::World world;

  // Spawn some entities
  auto entity1 = world.spawn().with(Position{0, 0}).with(Velocity{0.2, 0.2});
  auto entity2 = world.spawn().with(Position{10, 10}).with(Velocity{-1, 0});

  // Add physics system
  world.addSystem("Physics", [](ste::World &world) {
    auto time = world.getResource<ste::Time>();
    ste::Query<Position, Velocity> query(&world);

    for (auto [entity, pos, vel] : query) {
      pos.x += vel.dx * time->deltaSeconds;
      pos.y += vel.dy * time->deltaSeconds;
    }
  });

  // Initialize game timer
  ste::GameTimer timer(60); // 60 FPS target;

  bool running = true;
  while (running) {
    timer.update();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_SPACE:
          timer.togglePause();
          break;
        case SDLK_1:
          timer.setTimeScale(0.5f);
          break;
        case SDLK_2:
          timer.setTimeScale(1.0f);
          break;
        case SDLK_3:
          timer.setTimeScale(2.0f);
          break;
        }
      }
    }

    // Update world with actual delta time
    world.update(timer.getDeltaTime());

    // Clear the screen
    window->clearColor(fmod(entity1.get<Position>().x, 1.0f),
                       fmod(entity1.get<Position>().y, 1.0f), 0.2f, 1.0f);

    // Render world
    world.render();

    // Swap buffers
    window->swapBuffers();

    // Limit frame rate
    timer.limitFrameRate();
  }

  return 0;
}