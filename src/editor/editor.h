#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/engine.h>

namespace editor {

struct PlacementTool {
  glm::vec2 gridSize = {32.0f, 32.0f};
  glm::vec2 cursorPosition = glm::vec2(0.0f);
};

struct Tools {
  PlacementTool placementTool;
};

struct Level {
  std::string name;
  ste::Map map;
};

struct EditorState {
  Level currentLevel;
  Tools tools;
  bool debugMode = true;
};

struct PlaceObject {
  u32 objectId;
  glm::vec2 position;
};

bool setupDefaultResources(ste::World &world) {
  // Build a window
  auto window = ste::Window::builder()
                    .setTitle("Stabby Editor : v0.0.1")
                    .setSize(1280, 720)
                    .setVSync(true)
                    .setMSAA(8)
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
    return false;
  }

  // Create an asset manager
  auto assetManager = std::make_shared<ste::AssetManager>(assetLoader);
  assetManager->registerDefaults();

  // Create a renderer
  ste::Renderer2D::CreateInfo rendererCreateInfo;
  auto renderer = ste::Renderer2D::create(rendererCreateInfo);
  if (!renderer) {
    std::cerr << "Failed to create renderer!" << std::endl;
    return false;
  }

  // Setup the systems
  auto textRenderer = std::make_shared<ste::TextRenderer>(renderer);
  auto audioManager = std::make_shared<ste::AudioManager>();
  auto inputManager = std::make_shared<ste::InputManager>();

  // World tools
  auto camera =
      std::make_shared<ste::Camera2D>(window->getWidth(), window->getHeight());
  auto timer = std::make_shared<ste::GameTimer>(60.0f);

  // Add default resources
  world.addResource(assetLoader);
  world.addResource(assetManager);
  world.addResource(audioManager);
  world.addResource(camera);
  world.addResource(inputManager);
  world.addResource(renderer);
  world.addResource(textRenderer);
  world.addResource(timer);
  world.addResource(window);

  return true;
}

bool setupEditorResources(ste::World &world) {
  auto editorState = std::make_shared<EditorState>();

  world.addResource(editorState);

  return true;
}

bool loadAssets(ste::World &world) {
  auto assetManager = world.getResource<ste::AssetManager>();

  const std::pair<std::string, std::string> fontAssets[] = {
      {"font", "assets/fonts/better-vcr.ttf@11"},
  };

  // Load the editor assets synchronously
  for (const auto &[name, path] : fontAssets) {
    auto font = assetManager->load<ste::Font>(name, path);
    if (!font) {
      std::cerr << "Failed to load font: " << path << std::endl;
      return false;
    }
  }

  // Load the example default map
  try {
    auto defaultMap =
        assetManager->load<ste::Map>("default_map", "assets/maps/default.json");

    if (!defaultMap) {
      std::cerr << "Failed to load default map" << std::endl;
      return false;
    }

    // Set up the editor state with the loaded map
    auto editorState = world.getResource<EditorState>();
    editorState->currentLevel.name = "default";
    editorState->currentLevel.map = std::move(*defaultMap);

    // Debug print the map
    auto &map = editorState->currentLevel.map;
    for (const auto &layer : map.layers()) {
      std::cout << "Layer: " << layer.getName() << std::endl;
      for (const auto &tile : layer.getTiles()) {
        std::cout << "Tile: " << tile.getId() << " at " << tile.getPosition().x
                  << ", " << tile.getPosition().y << std::endl;
      }
    }

  } catch (const std::exception &e) {
    std::cerr << "Failed to load default map: " << e.what() << std::endl;
    return false;
  }

  return true;
}

bool setup(ste::World &world) {
  if (!setupDefaultResources(world)) {
    std::cerr << "Failed to setup default resources!" << std::endl;
    return false;
  }

  if (!setupEditorResources(world)) {
    std::cerr << "Failed to setup editor resources!" << std::endl;
    return false;
  }

  if (!loadAssets(world)) {
    std::cerr << "Failed to load assets!" << std::endl;
    return false;
  }

  return true;
}

}; // namespace editor

namespace systems {

using namespace editor;

void inputManagement(ste::World &world) {
  auto inputManager = world.getResource<ste::InputManager>();
  inputManager->update();

  // If the 0 key is pressed, toggle debug mode
  if (inputManager->isKeyPressed(ste::Input::Num0)) {
    auto editorState = world.getResource<EditorState>();
    editorState->debugMode = !editorState->debugMode;
  }
}

void placementTool(ste::World &world) {
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
    world.emit(PlaceObject{0, placementTool.cursorPosition});
  }
}

void renderDebugStats(ste::World &world) {
  auto editorState = world.getResource<EditorState>();
  auto renderer = world.getResource<ste::Renderer2D>();
  auto textRenderer = world.getResource<ste::TextRenderer>();
  auto window = world.getResource<ste::Window>();
  auto assetManager = world.getResource<ste::AssetManager>();

  auto font = assetManager->get<ste::Font>("font");

  if (!editorState->debugMode)
    return;

  // Draw the FPS counter
  auto timer = world.getResource<ste::GameTimer>();
  auto camera = world.getResource<ste::Camera2D>();

  // Set up the UI projection
  const auto uiProjection =
      glm::ortho(0.0f, (float)window->getWidth(), (float)window->getHeight(),
                 0.0f, -1.0f, 1.0f);

  char *truncatedFps;
  asprintf(&truncatedFps, "%.4f", timer->getFPS());

  auto fpsText =
      textRenderer->createText(*font, "FPS: " + std::string(truncatedFps),
                               {16.0f, 16.0f}, {1.0f, 1.0f, 1.0f, 1.0f});

  renderer->beginScene(uiProjection);

  // Draw a background for the FPS counter based on the width and height of the
  // text
  auto textSize = fpsText.getSize();
  auto textPosition = fpsText.getPosition();
  renderer->drawQuad({textPosition.x - 8.0f, textPosition.y - 8.0f, 0.0f},
                     {textSize.x + 16.0f, textSize.y + 16.0f},
                     {0.0f, 0.0f, 0.0f, 0.5f}, 0.0f, {0.0f, 0.0f}, 1.0f,
                     {1.0f, 1.0f, 1.0f, 1.0f});

  // Render the FPS counter
  fpsText.render();

  renderer->endScene();
}

void renderTools(ste::World &world) {
  auto camera = world.getResource<ste::Camera2D>();
  auto editorState = world.getResource<EditorState>();
  auto renderer = world.getResource<ste::Renderer2D>();

  auto &placementTool = editorState->tools.placementTool;

  // Begin drawing the scene
  renderer->beginScene(camera->getViewProjectionMatrix());

  // Draw the placement tool cursor
  renderer->drawQuad(
      {placementTool.cursorPosition.x, placementTool.cursorPosition.y, 0.0f},
      {placementTool.gridSize.x, placementTool.gridSize.y},
      {1.0f, 1.0f, 1.0f, 0.1f}, 0.0f, {0.0f, 0.0f}, 2.0f,
      {1.0f, 1.0f, 1.0f, 1.0f});

  // End drawing the scene
  renderer->endScene();
}

void renderMap(ste::World &world) {
  auto camera = world.getResource<ste::Camera2D>();
  auto editorState = world.getResource<EditorState>();
  auto map = &editorState->currentLevel.map;
  auto renderer = world.getResource<ste::Renderer2D>();

  // Begin drawing the scene
  renderer->beginScene(camera->getViewProjectionMatrix());

  // Draw the map
  for (const auto &layer : map->layers()) {
    for (const auto &tile : layer.getTiles()) {
      auto position = tile.getPosition();
      auto size = tile.getSize();
      renderer->drawQuad({position.x, position.y, 0.0f}, {size.x, size.y},
                         {1.0f, 1.0f, 1.0f, 1.0f});
    }

    // End drawing the scene
    renderer->endScene();
  }
}

} // namespace systems

namespace handlers {

void objectPlacement(ste::World &world, const editor::PlaceObject &event) {
  auto editorState = world.getResource<editor::EditorState>();
  auto map = &editorState->currentLevel.map;

  auto gridSize = editorState->tools.placementTool.gridSize;

  // If there's a tile at the position + half grid since it's center
  // positions, remove it
  if (auto tile = map->getTileAt({event.position.x + gridSize.x / 2.0f,
                                  event.position.y + gridSize.y / 2.0f})) {
    std::cout << "Removing object at: " << event.position.x << ", "
              << event.position.y << std::endl;
    map->removeTile("background", tile->tileId);
  } else {
    std::cout << "Placing object at: " << event.position.x << ", "
              << event.position.y << std::endl;
    auto tileId = map->addTile("background", 0, event.position, gridSize);
  }
}

} // namespace handlers
