#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace ste {

// Forward declarations
class Font;

class FontSystem : public std::enable_shared_from_this<FontSystem> {
public:
  struct CreateInfo {
    std::string errorMsg;
    bool success = true;
  };

  // Singleton access - the engine should own this system
  static std::optional<FontSystem> create(CreateInfo &createInfo);
  ~FontSystem();

  FontSystem(const FontSystem &) = delete;
  FontSystem &operator=(const FontSystem &) = delete;
  FontSystem(FontSystem &&other) noexcept;
  FontSystem &operator=(FontSystem &&other) noexcept;

  // Font loading configuration
  struct FontConfig {
    uint32_t fontSize = 32;    // Base size for SDF generation
    uint32_t atlasSize = 1024; // Texture atlas size
    uint32_t padding = 4;      // SDF padding
    bool generateMips = true;  // Generate mipmaps for atlas
    std::string errorMsg;      // Error reporting
    bool success = true;
  };

  // Load a font with the given configuration
  std::shared_ptr<Font> loadFont(const std::string &path, FontConfig &config);

private:
  explicit FontSystem(FT_Library ftLibrary);

  FT_Library m_ftLibrary;
};

} // namespace ste