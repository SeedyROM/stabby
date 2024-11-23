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
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    return -1;
  }

  // Set OpenGL attributes
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  // Create window with OpenGL context
  SDL_Window *window = SDL_CreateWindow("SDL2 + OpenGL", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, 800, 600,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  if (!window) {
    std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return -1;
  }

  // Create OpenGL context
  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::cerr << "OpenGL context creation failed: " << SDL_GetError()
              << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  ste::World world;

  // Spawn some entities
  auto entity1 = world.spawn().with(Position{0, 0}).with(Velocity{1, 1});
  auto entity2 = world.spawn().with(Position{10, 10}).with(Velocity{-1, 0});

  // Add physics system
  world.addSystem("Physics", [](ste::World &world) {
    auto time = world.getResource<ste::Time>();
    ste::Query<Position, Velocity> query(&world);

    for (auto &&[entity, pos, vel] : query) {
      pos.x += vel.dx * time->deltaSeconds;
      pos.y += vel.dy * time->deltaSeconds;
    }
  });

  // Optional: Enable VSync
  SDL_GL_SetSwapInterval(1);

  // Initialize game timer
  GameTimer timer(60); // 60 FPS target

  bool running = true;
  while (running) {
    timer.tick();

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
          timer.slowMotion();
          break;
        case SDLK_2:
          timer.normalSpeed();
          break;
        case SDLK_3:
          timer.fastForward();
          break;
        case SDLK_f:
          timer.setShowFPS(!timer.getFPS());
          break;
        }
      }
    }

    // Update world with actual delta time
    world.update(timer.getDeltaTime());

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(fmod(entity1.get<Position>().x, 1.0f), 0.3f, 0.3f, 1.0f);

    // Render world
    world.render();

    // Swap buffers
    SDL_GL_SwapWindow(window);

    // Limit frame rate
    timer.limitFrameRate();
  }

  // Cleanup
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}