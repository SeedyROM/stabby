#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "engine/assets/asset_manager.h"
#include "engine/rendering/texture.h"

namespace ste {

// Forward declarations
class FontSystem;

struct GlyphMetrics {
  glm::vec2 size;      // Size in pixels
  glm::vec2 bearing;   // Offset from baseline
  glm::vec2 advance;   // Advance to next glyph
  glm::vec4 texCoords; // Atlas UV coordinates
  float scale;         // SDF scale factor
};

class Font {
  friend class FontSystem;

public:
  ~Font();
  Font(const Font &) = delete;
  Font &operator=(const Font &) = delete;
  Font(Font &&other) noexcept;
  Font &operator=(Font &&other) noexcept;

  // Core functionality
  const GlyphMetrics &getGlyph(char c) const;
  const Texture &getAtlasTexture() const { return m_atlasTexture; }
  float getLineHeight() const { return m_lineHeight; }
  float getBaseline() const { return m_baseline; }

  // Text measurement
  glm::vec2 measureText(const std::string &text, float scale = 1.0f) const;
  float getKerning(char first, char second) const;

private:
  // Only FontSystem can construct Fonts
  Font(Texture &&atlas, std::unordered_map<char, GlyphMetrics> &&glyphs,
       float lineHeight, float baseline,
       const std::shared_ptr<FontSystem> &system);

  Texture m_atlasTexture;
  std::unordered_map<char, GlyphMetrics> m_glyphs;
  float m_lineHeight;
  float m_baseline;
  std::unordered_map<uint32_t, float> m_kerningTable;
  std::shared_ptr<FontSystem> m_system; // Keep system alive while font exists
};

} // namespace ste