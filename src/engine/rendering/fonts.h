#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <memory>
#include <string>
#include <unordered_map>

#include "renderer_2d.h"

namespace ste {

class FontLibrary {
public:
  static FontLibrary &get() {
    static FontLibrary instance;
    return instance;
  }

  ~FontLibrary();

  FT_Library getLibrary() const { return m_library; }
  bool isValid() const { return m_library != nullptr; }
  const std::string &getError() const { return m_error; }

private:
  FontLibrary();
  FT_Library m_library{nullptr};
  std::string m_error;
};

// Font Atlas for caching glyphs
class FontAtlas {
public:
  struct GlyphInfo {
    float u0, v0; // Texture coordinates
    float u1, v1;
    int bearingX, bearingY; // Offset from baseline
    int advance;            // Horizontal advance
    int width, height;      // Size of glyph
  };

  FontAtlas(uint32_t width = 1024, uint32_t height = 1024);
  ~FontAtlas();

  // Delete copy operations
  FontAtlas(const FontAtlas &) = delete;
  FontAtlas &operator=(const FontAtlas &) = delete;

  // Add move operations
  FontAtlas(FontAtlas &&other) noexcept;
  FontAtlas &operator=(FontAtlas &&other) noexcept;

  // Get atlas texture
  uint32_t getTexture() const { return m_textureId; }

  // Add glyph to atlas
  bool addGlyph(uint32_t codepoint, const uint8_t *bitmap, uint32_t width,
                uint32_t height, int bearingX, int bearingY, int advance);

  // Get glyph info
  const GlyphInfo *getGlyph(uint32_t codepoint) const;

private:
  bool findSpace(uint32_t width, uint32_t height, uint32_t &x, uint32_t &y);

  uint32_t m_textureId;
  uint32_t m_width, m_height;
  uint32_t m_currentX{0}, m_currentY{0};
  uint32_t m_rowHeight{0};

  std::unordered_map<uint32_t, GlyphInfo> m_glyphs;
};

// Font class
class Font {
public:
  struct CreateInfo {
    std::string errorMsg;
    bool success = true;
    uint32_t size = 16; // Font size in pixels
  };

  static std::optional<Font> createFromFile(const std::string &path,
                                            CreateInfo &createInfo);

  ~Font();
  Font(const Font &) = delete;
  Font &operator=(const Font &) = delete;
  Font(Font &&other) noexcept;
  Font &operator=(Font &&other) noexcept;

  // Get metrics
  float getKerning(uint32_t first, uint32_t second) const;
  float getLineHeight() const { return m_lineHeight; }
  float getBaseline() const { return m_baseline; }

  // Load glyph into atlas
  bool cacheGlyph(uint32_t codepoint);
  const FontAtlas::GlyphInfo *getGlyphInfo(uint32_t codepoint) const;
  uint32_t getAtlasTexture() const { return m_atlas.getTexture(); }

private:
  Font(FT_Face face, uint32_t size);

  FT_Face m_face{nullptr};
  FontAtlas m_atlas;
  float m_lineHeight{0};
  float m_baseline{0};
};

// Text renderer
class TextRenderer {
public:
  struct TextMetrics {
    float width;    // Total width
    float height;   // Total height
    float baseline; // Distance from top to baseline
  };

  explicit TextRenderer(std::shared_ptr<Renderer2D> renderer);

  // Calculate text metrics
  TextMetrics calculateMetrics(const Font &font, const std::string &text) const;

  // Render text
  void renderText(Font &font, const std::string &text,
                  const glm::vec2 &position,
                  const glm::vec4 &color = {1, 1, 1, 1});

private:
  std::shared_ptr<Renderer2D> m_renderer;
};

} // namespace ste