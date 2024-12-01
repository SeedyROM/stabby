#include "font.h"
#include "font_system.h"

namespace ste {

Font::Font(Texture &&atlas, std::unordered_map<char, GlyphMetrics> &&glyphs,
           float lineHeight, float baseline,
           const std::shared_ptr<FontSystem> &system)
    : m_atlasTexture(std::move(atlas)), m_glyphs(std::move(glyphs)),
      m_lineHeight(lineHeight), m_baseline(baseline), m_system(system) {}

Font::~Font() = default;

Font::Font(Font &&other) noexcept
    : m_atlasTexture(std::move(other.m_atlasTexture)),
      m_glyphs(std::move(other.m_glyphs)), m_lineHeight(other.m_lineHeight),
      m_baseline(other.m_baseline),
      m_kerningTable(std::move(other.m_kerningTable)),
      m_system(std::move(other.m_system)) {}

Font &Font::operator=(Font &&other) noexcept {
  if (this != &other) {
    m_atlasTexture = std::move(other.m_atlasTexture);
    m_glyphs = std::move(other.m_glyphs);
    m_lineHeight = other.m_lineHeight;
    m_baseline = other.m_baseline;
    m_kerningTable = std::move(other.m_kerningTable);
    m_system = std::move(other.m_system);
  }
  return *this;
}

const GlyphMetrics &Font::getGlyph(char c) const {
  auto it = m_glyphs.find(c);
  if (it != m_glyphs.end()) {
    return it->second;
  }

  // Return space character metrics if glyph not found
  static const GlyphMetrics defaultGlyph{
      {0.0f, 0.0f},                 // size
      {0.0f, 0.0f},                 // bearing
      {m_lineHeight * 0.25f, 0.0f}, // advance (quarter em space)
      {0.0f, 0.0f, 0.0f, 0.0f},     // texCoords
      1.0f                          // scale
  };
  return defaultGlyph;
}

float Font::getKerning(char first, char second) const {
  // Create a unique key for the character pair
  uint32_t key =
      (static_cast<uint32_t>(first) << 16) | static_cast<uint32_t>(second);

  auto it = m_kerningTable.find(key);
  if (it != m_kerningTable.end()) {
    return it->second;
  }
  return 0.0f;
}

glm::vec2 Font::measureText(const std::string &text, float scale) const {
  if (text.empty()) {
    return {0.0f, m_lineHeight * scale};
  }

  float width = 0.0f;
  float height = m_lineHeight * scale;
  float maxBearing = 0.0f;

  char prevChar = '\0';
  for (char c : text) {
    // Apply kerning
    width += getKerning(prevChar, c) * scale;

    const auto &glyph = getGlyph(c);
    width += glyph.advance.x * scale;

    // Track maximum height from baseline
    float glyphHeight = glyph.size.y + glyph.bearing.y;
    maxBearing = std::max(maxBearing, glyphHeight);

    prevChar = c;
  }

  // Final height is maximum of line height and highest glyph
  height = std::max(height, maxBearing * scale);

  return {width, height};
}

} // namespace ste