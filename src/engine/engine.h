#pragma once

#include "engine/assets/assets.h"
#include "engine/audio/audio.h"
#include "engine/input/input_manager.h"
#include "engine/rendering/rendering.h"
#include "engine/time/game_timer.h"
#include "engine/utils.h"
#include "engine/world/world.h"
#include "scene/scene.h"

namespace ste {

template <typename T>
concept HasCreateInfo = requires(T t, typename T::CreateInfo ci) {
  typename T::CreateInfo;
  requires std::is_class_v<typename T::CreateInfo>;
  { ci.errorMsg } -> std::convertible_to<std::string>;
  { ci.success } -> std::convertible_to<bool>;
  { T::create(ci) } -> std::same_as<std::optional<T>>;
};

// TODO(SeedyROM): This code is TRASH, remove it.
template <HasCreateInfo T>
std::shared_ptr<T>
createSharedResource(std::optional<T> &opt, const char *resourceName,
                     const typename T::CreateInfo &createInfo) {
  if (!opt) {
    std::string message = std::string("Failed to create ") + resourceName;
    if (!createInfo.errorMsg.empty()) {
      message += ": " + createInfo.errorMsg;
    }
    throw std::runtime_error(message);
  }
  return std::make_shared<T>(std::move(*opt));
}

// TODO(SeedyROM): This code is TRASH, remove it.
std::optional<std::shared_ptr<ste::SceneManager>>
createSceneManager(const std::shared_ptr<ste::Window> window);

} // namespace ste