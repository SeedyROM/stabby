#pragma once

#include <string>

namespace ste {

constexpr std::string getAssetPath(const std::string &filename) {
  return std::string(ASSET_PATH) + "/" + filename;
}

} // namespace ste