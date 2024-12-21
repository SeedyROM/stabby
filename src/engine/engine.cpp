#include "engine.h"

namespace ste {

std::optional<std::shared_ptr<ste::SceneManager>>
createSceneManager(const std::shared_ptr<ste::Window> window) {
  try {
    auto sceneManager = std::make_shared<ste::SceneManager>();

    // Create configs
    ste::Renderer2D::CreateInfo rendererCreateInfo;
    ste::AssetLoader::CreateInfo assetCreateInfo;

    // Create resources
    auto renderer = ste::Renderer2D::create(rendererCreateInfo);
    auto assetLoader = ste::AssetLoader::create(assetCreateInfo);

    // Set up manager resources
    sceneManager->setRenderer(
        createSharedResource(renderer, "renderer", rendererCreateInfo));
    sceneManager->setAssetLoader(
        createSharedResource(assetLoader, "asset loader", assetCreateInfo));
    sceneManager->setCamera(std::make_shared<ste::Camera2D>(
        window->getWidth(), window->getHeight()));
    sceneManager->setAudioManager(std::make_shared<ste::AudioManager>());

    return sceneManager;
  } catch (const std::exception &e) {
    std::cerr << "Error setting up scene manager: " << e.what() << std::endl;
    return std::nullopt;
  }
}

} // namespace ste