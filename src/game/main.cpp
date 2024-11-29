#include <filesystem>
#include <iostream>

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/engine.h>

// Components to be used in the ECS
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
    // Create a random float between -0.2 and 0.2
    rotation =
        (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f -
         1.0f) *
        0.8f;
  }
};

int main(int argc, char *argv[]) {
  // Create a window with the window builder.
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

  // Setup the audio system
  ste::AudioManager audio;

  auto &audioEngine = audio.getEngine();

  // Create a camera
  ste::Camera2D camera(window->getWidth(), window->getHeight());

  const float CAMERA_SPEED =
      500.0f; // Adjust this to control camera acceleration
  bool keys[SDL_NUM_SCANCODES] = {false};

  // Create a renderer
  ste::Renderer2D::CreateInfo rendererCreateInfo;
  auto renderer = ste::Renderer2D::create(rendererCreateInfo);
  if (!renderer) {
    std::cerr << "Failed to create renderer: " << rendererCreateInfo.errorMsg
              << std::endl;
    return -1;
  }

  // Create an asset manager
  ste::AssetManager::CreateInfo assetCreateInfo;
  auto assetManager = ste::AssetManager::create(assetCreateInfo);
  if (!assetManager) {
    std::cerr << "Failed to create asset manager: " << assetCreateInfo.errorMsg
              << std::endl;
    return -1;
  }

  // Load a texture
  ste::AssetHandle<ste::Texture> textureHandle;
  try {
    textureHandle = assetManager->load<ste::Texture>(
        ste::getAssetPath("textures/albert.png"));
    if (!textureHandle) {
      std::cerr << "Failed to load texture" << std::endl;
      return -1;
    }
  } catch (const std::exception &e) {
    std::cerr << "Failed to load texture: " << e.what() << std::endl;
    return -1;
  }

  // Enable alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Create a world
  ste::World world;

  // Add physics system
  world.addSystem("Physics", [](ste::World &world) {
    auto time = world.getResource<ste::Time>();
    ste::Query<Transform, Velocity, Spinny> query(&world);

    for (auto [entity, transform, vel, spinny] : query) {
      transform.position.x += vel.dx * time->deltaSeconds;
      transform.position.y += vel.dy * time->deltaSeconds;
      transform.rotation += spinny.rotation * time->deltaSeconds;

      // Wrap around screen edges
      if (transform.position.x < 0)
        transform.position.x = 1280;
      if (transform.position.x > 1280)
        transform.position.x = 0;
      if (transform.position.y < 0)
        transform.position.y = 720;
      if (transform.position.y > 720)
        transform.position.y = 0;
    }
  });

  // Create texture info from the handle
  ste::Renderer2D::TextureInfo texInfo{textureHandle->getId(),
                                       textureHandle->getWidth(),
                                       textureHandle->getHeight()};

  world.addSystem(
      "Rendering",
      [&renderer, &window, &texInfo](ste::World &world) {
        ste::Query<Transform, Spinny> query(&world);

        for (auto [entity, transform, spinny] : query) {

          // Draw the textured quad
          renderer->drawTexturedQuad(
              {transform.position.x,
               window->getHeight() -
                   transform.position.y}, // Position (flip Y for OpenGL)
              texInfo,
              transform.scale,          // Size
              {1.0f, 1.0f, 1.0f, 1.0f}, // Color tint
              transform.rotation,       // Rotation
              {0.0f, 0.0f, 1.0f, 1.0f}  // Texture coordinates (full texture)
          );
        }
      },
      0, true);

  // Initialize game timer
  ste::GameTimer timer(60);

  audioEngine.playMusic("./assets/music/paniots-nine.wav", true);

  bool running = true;
  while (running) {
    timer.update();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_1:
          timer.setTimeScale(0.5f);
          audioEngine.setSpeed(0.5f);
          audioEngine.playSound("./assets/sfx/slowdown.wav");
          break;
        case SDLK_2:
          timer.setTimeScale(1.0f);
          audioEngine.setSpeed(1.0f);
          audioEngine.playSound("./assets/sfx/slowdown.wav");
          break;
        case SDLK_3:
          timer.setTimeScale(2.0f);
          audioEngine.setSpeed(2.0f);
          audioEngine.playSound("./assets/sfx/slowdown.wav");
          break;
        case SDLK_SPACE:
          audioEngine.playSound("./assets/sfx/real-trap-shit.wav");

          // Spawn textured entities
          for (int i = 0; i < 50; i++) {
            float size = 64.0f + (rand() % 192); // Random between 64 and 256
            world.spawn()
                .with(Transform{
                    {static_cast<float>(rand() % 1280),
                     static_cast<float>(rand() % 720)}, // Random position
                    {size, size},                       // Random square size
                    0.0f})
                .with(Velocity{
                    static_cast<float>(rand() % 100 - 50), // Random X velocity
                    static_cast<float>(rand() % 100 - 50)  // Random Y velocity
                })
                .with(Spinny{});
          }
          break;
        }

        keys[event.key.keysym.scancode] = true;
      }
      if (event.type == SDL_KEYUP) {
        keys[event.key.keysym.scancode] = false;
      }
    }

    // Update camera movement based on held keys
    glm::vec2 moveDir(0.0f, 0.0f);
    if (keys[SDL_SCANCODE_W])
      moveDir.y += 1.0f;
    if (keys[SDL_SCANCODE_S])
      moveDir.y -= 1.0f;
    if (keys[SDL_SCANCODE_A])
      moveDir.x -= 1.0f;
    if (keys[SDL_SCANCODE_D])
      moveDir.x += 1.0f;

    // Zoom control
    if (keys[SDL_SCANCODE_Q]) {
      camera.addZoom(-0.1f * timer.getDeltaTime());
    }
    if (keys[SDL_SCANCODE_E]) {
      camera.addZoom(0.1f * timer.getDeltaTime());
    }

    // Normalize diagonal movement
    if (glm::length(moveDir) > 0.0f) {
      moveDir = glm::normalize(moveDir);
    }

    // Add velocity based on input
    camera.addVelocity(moveDir * CAMERA_SPEED * timer.getDeltaTime());

    // Update the camera
    camera.update(timer.getDeltaTime());

    // Update world with actual delta time
    world.update(timer.getDeltaTime());

    // Clear the screen
    window->clearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Begin scene
    renderer->beginScene(camera.getViewProjectionMatrix());

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