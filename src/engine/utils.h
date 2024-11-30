#pragma once

#include <string>

namespace ste {

inline std::string getAssetPath(const std::string &filename) {
  return std::string(ASSET_PATH) + "/" + filename;
}

} // namespace ste