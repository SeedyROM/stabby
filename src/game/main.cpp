#include <iostream>

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/engine.h>

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
    // Create a random float between -speed and speed
    this->rotation = (rand() % 1000) / 1000.0f * 2.0f;
  }
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

  // Add physics system
  world.addSystem("Physics", [](ste::World &world) {
    auto time = world.getResource<ste::Time>();
    ste::Query<Transform, Velocity, Spinny> query(&world);

    for (auto [entity, transform, vel, spinny] : query) {
      transform.position.x += vel.dx * time->deltaSeconds;
      transform.position.y += vel.dy * time->deltaSeconds;
      spinny.rotation += 0.01f;
      transform.rotation += spinny.rotation;
    }
  });

  world.addSystem(
      "Rendering",
      [&renderer, &window](ste::World &world) {
        ste::Query<Transform, Spinny> query(&world);

        for (auto [entity, transform, spinny] : query) {
          renderer->drawQuad({transform.position.x,
                              window->getHeight() - transform.position.y},
                             {100.0f, 100.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
                             spinny.rotation);
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
        case SDLK_s:
          // Spawn 100 entities
          for (int i = 0; i < 100; i++) {
            world.spawn()
                .with(Transform{
                    {rand() % 1280, rand() % 720}, {100.0f, 100.0f}, 0.0f})
                // Random velocity
                .with(Velocity{(rand() % 100) - 50.0f, (rand() % 100) - 50.0f})
                .with(Spinny{});
          }
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