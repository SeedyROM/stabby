#include <iostream>

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/engine.h>

struct Position {
  float x, y;
};

struct Velocity {
  float dx, dy;
};

int main(int argc, char *argv[]) {
  auto window = ste::Window::builder()
                    .setTitle("My Window")
                    .setSize(1280, 720)
                    .setVSync(true)
                    .build()
                    .value_or(nullptr);

  if (!window) {
    std::cerr << "Failed to create window!" << std::endl;
    return -1;
  }

  // MVP matrices
  glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f, -1.0f, 1.0f);
  glm::mat4 view = glm::mat4(1.0f); // Identity for simple 2D
  glm::mat4 viewProjection = projection * view;

  // Create a renderer
  ste::Renderer2D::CreateInfo createInfo;
  auto renderer = ste::Renderer2D::create(createInfo);
  if (!renderer) {
    std::cerr << "Failed to create renderer: " << createInfo.errorMsg
              << std::endl;
    return -1;
  }

  // Create a world
  ste::World world;

  // Spawn some entities
  auto entity1 = world.spawn().with(Position{0, 0}).with(Velocity{30, 1});
  auto entity2 = world.spawn().with(Position{100, 10}).with(Velocity{15, 20});

  // Add physics system
  world.addSystem("Physics", [](ste::World &world) {
    auto time = world.getResource<ste::Time>();
    ste::Query<Position, Velocity> query(&world);

    for (auto [entity, pos, vel] : query) {
      pos.x += vel.dx * time->deltaSeconds;
      pos.y += vel.dy * time->deltaSeconds;
    }
  });

  world.addSystem(
      "Rendering",
      [&renderer, &window](ste::World &world) {
        ste::Query<Position> query(&world);

        for (auto [entity, pos] : query) {
          renderer->drawQuad({pos.x, window->getHeight() - pos.y},
                             {100.0f, 100.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
        }
      },
      0, true);

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
    window->clearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Begin scene
    renderer->beginScene(viewProjection);

    // Render world
    world.render();

    // End scene
    renderer->endScene();

    // Swap buffers
    window->swapBuffers();

    // Limit frame rate
    timer.limitFrameRate();
  }

  return 0;
}