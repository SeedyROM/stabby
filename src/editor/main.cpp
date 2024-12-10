#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/engine.h>

struct PlacementTool {
  glm::vec2 gridSize = {64.0f, 64.0f};
  glm::vec2 cursorPosition = glm::vec2(0.0f);
};

struct Tools {
  PlacementTool placementTool;
};

struct Level {
  std::string name;
};

struct EditorState {
  Level currentLevel;
  Tools tools;
  bool debugMode = true;
};

namespace Events {
struct PlaceObject {
  u32 objectId;
  glm::vec2 position;
};

} // namespace Events

int main(int argc, char *argv[]) {
  // Build a window
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

  // Create an asset loader
  ste::AssetLoader::CreateInfo assetCreateInfo;
  auto assetLoader = ste::AssetLoader::create(assetCreateInfo);
  if (!assetLoader) {
    std::cerr << "Failed to create asset loader!" << std::endl;
    return -1;
  }

  // Load a font
  auto font = assetLoader->load<ste::Font>(
      ste::getAssetPath("fonts/better-vcr.ttf@13"));

  // Create a renderer
  ste::Renderer2D::CreateInfo rendererCreateInfo;
  auto renderer = ste::Renderer2D::create(rendererCreateInfo);
  if (!renderer) {
    std::cerr << "Failed to create renderer!" << std::endl;
    return -1;
  }

  // Setup the systems
  auto textRenderer = std::make_shared<ste::TextRenderer>(*renderer);
  ste::AudioManager audioManager;
  auto inputManager = std::make_shared<ste::InputManager>();

  // World tools
  auto camera =
      std::make_shared<ste::Camera2D>(window->getWidth(), window->getHeight());
  auto editorState = std::make_shared<EditorState>();
  auto timer = std::make_shared<ste::GameTimer>(60.0f);

  // Setup the world
  ste::World world;

  // Add resources
  world.addResource(camera);
  world.addResource(editorState);
  world.addResource(inputManager);
  world.addResource(timer);

  // Add systems
  world.addSystem("Input Manager", [](ste::World &world) {
    auto inputManager = world.getResource<ste::InputManager>();
    inputManager->update();

    // If the 0 key is pressed, toggle debug mode
    if (inputManager->isKeyPressed(ste::Input::Num0)) {
      auto editorState = world.getResource<EditorState>();
      editorState->debugMode = !editorState->debugMode;
    }
  });

  world.addSystem("Placement Tool", [](ste::World &world) {
    auto editorState = world.getResource<EditorState>();
    auto camera = world.getResource<ste::Camera2D>();
    auto inputManager = world.getResource<ste::InputManager>();

    auto &placementTool = editorState->tools.placementTool;

    // Get the mouse position
    glm::vec2 mousePosition = inputManager->getMousePosition();

    // Get the world space cursor position
    placementTool.cursorPosition =
        camera->screenToWorld({mousePosition.x, mousePosition.y});

    // Lock the cursor to a grid
    placementTool.cursorPosition.x =
        std::floor(placementTool.cursorPosition.x / placementTool.gridSize.x) *
        placementTool.gridSize.x;
    placementTool.cursorPosition.y =
        std::floor(placementTool.cursorPosition.y / placementTool.gridSize.y) *
        placementTool.gridSize.y;

    // If the user clicks emit an event
    if (inputManager->isMouseButtonPressed(ste::Input::MouseLeft)) {
      world.emit(Events::PlaceObject{0, placementTool.cursorPosition});
    }
  });

  // Subscribe to event object placement
  world.subscribe<Events::PlaceObject>([](const auto &event) {
    std::cout << "Placing object at: " << event.position.x << ", "
              << event.position.y << std::endl;
  });

  // Add the debug rendering
  world.addSystem(
      "Render Debug",
      [&renderer, &textRenderer, &font, &window](ste::World &world) {
        auto editorState = world.getResource<EditorState>();
        if (!editorState->debugMode)
          return;

        // Draw the FPS counter
        auto timer = world.getResource<ste::GameTimer>();
        auto camera = world.getResource<ste::Camera2D>();

        // Set up the UI projection
        auto uiProjection = glm::ortho(0.0f, (float)window->getWidth(), 0.0f,
                                       (float)window->getHeight(), -1.0f, 1.0f);

        renderer->beginScene(uiProjection);

        textRenderer->renderText(*font,
                                 " FPS: " + std::to_string(timer->getFPS()),
                                 {10.0f, window->getHeight() - 13.0f - 13.0f},
                                 {1.0f, 1.0f, 1.0f, 1.0f});

        renderer->endScene();
      },
      1, true);

  // Add the tool rendering
  world.addSystem(
      "Render Tools",
      [&renderer](ste::World &world) {
        auto camera = world.getResource<ste::Camera2D>();
        auto editorState = world.getResource<EditorState>();

        auto &placementTool = editorState->tools.placementTool;

        // Begin drawing the scene
        renderer->beginScene(camera->getViewProjectionMatrix());

        // Draw the placement tool cursor
        renderer->drawQuad(
            {placementTool.cursorPosition.x + (placementTool.gridSize.x / 2.0f),
             placementTool.cursorPosition.y + (placementTool.gridSize.y / 2.0f),
             0.0f},
            {placementTool.gridSize.x, placementTool.gridSize.y},
            {1.0f, 1.0f, 1.0f, 0.1f}, 0.0f, 0.1f, {1.0f, 1.0f, 1.0f, 1.0f});

        // End drawing the scene
        renderer->endScene();
      },
      0, true);

  // Editor loop
  bool running = true;
  while (running) {
    // Update the game timer
    timer->update();

    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    // Update
    world.update(timer->getDeltaTime());

    // Render
    window->clearColor(0.361f, 0.361f, 0.471f, 1.0f);
    world.render();
    window->swapBuffers();

    // Cap the frame rate
    timer->limitFrameRate();
  }

  return 0;
}