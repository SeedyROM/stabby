#include <iostream>
#include <map>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "editor.h"

int main(int argc, char *argv[]) {
  // Create the editor in the world
  ste::World world;
  editor::setup(world);

  world.addSystem("Input Management", systems::inputManagement);
  world.addSystem("Placement Tool", systems::placementTool);

  world.addRenderSystem("Render Map", systems::renderMap);
  world.addRenderSystem("Render Tools", systems::renderTools);
  world.addRenderSystem("Render Debug Stats", systems::renderDebugStats);

  world.subscribe<editor::PlaceObject>(handlers::objectPlacement);

  // Loop variables
  bool running = true;
  auto window = world.getResource<ste::Window>();
  auto timer = world.getResource<ste::GameTimer>();

  // The editor loop...
  while (running) {
    // Update the timer and poll events
    timer->update();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    // Update the world and it's systems
    world.update(timer->getDeltaTime());

    // Clear the window
    window->clearColor(0.361f, 0.361f, 0.471f, 1.0f);

    // Render the world
    world.render();

    // Swap the buffers
    window->swapBuffers();

    // Limit the frame rate
    timer->limitFrameRate();
  }

  return 0;
}