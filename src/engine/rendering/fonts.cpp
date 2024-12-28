#include "fonts.h"

#include <iostream>

namespace ste {

FontLibrary::FontLibrary() {
  if (FT_Init_FreeType(&m_library) != 0) {
    m_error = "Failed to initialize FreeType";
  }
}

FontLibrary::~FontLibrary() {
  if (m_library) {
    FT_Done_FreeType(m_library);
  }
}

FontAtlas::FontAtlas(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {

  // Create texture atlas
  glGenTextures(1, &m_textureId);
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cerr << "Failed to generate texture. Error: " << error << std::endl;
    return;
  }

  // Try binding BEFORE initialization
  glBindTexture(GL_TEXTURE_2D, m_textureId);
  error = glGetError();
  if (error != GL_NO_ERROR) {
    return;
  }

  // Initialize with empty data
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED,
               GL_UNSIGNED_BYTE, nullptr);
  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cerr << "Error in glTexImage2D: " << error << std::endl;
    return;
  }

  // Now check if texture is valid
  GLboolean isTexture = glIsTexture(m_textureId);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Ensure texture uses red channel as alpha
  GLint swizzleMask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
  glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

  // Verify texture is still valid
  isTexture = glIsTexture(m_textureId);

  // Unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);
}

FontAtlas::~FontAtlas() {
  if (m_textureId) {
    glDeleteTextures(1, &m_textureId);
  }
}

bool FontAtlas::findSpace(uint32_t width, uint32_t height, uint32_t &x,
                          uint32_t &y) {
  // If we can't fit on current row, move to next row
  if (m_currentX + width > m_width) {
    m_currentX = 0;
    m_currentY += m_rowHeight;
    m_rowHeight = 0;
  }

  // Check if we have room vertically
  if (m_currentY + height > m_height) {
    return false;
  }

  // Update position and row height
  x = m_currentX;
  y = m_currentY;
  m_currentX += width;
  m_rowHeight = std::max(m_rowHeight, height);
  return true;
}

bool FontAtlas::addGlyph(uint32_t codepoint, const uint8_t *bitmap,
                         uint32_t width, uint32_t height, int bearingX,
                         int bearingY, int advance) {

  if (m_textureId == 0) {
    std::cerr << "Invalid texture ID (zero)" << std::endl;
    return false;
  }

  // Check texture validity at start of method
  GLboolean isTexture = glIsTexture(m_textureId);
  if (!isTexture) {
    std::cerr << "Texture " << m_textureId << " is invalid at start of addGlyph"
              << std::endl;
    return false;
  }

  // Find space in atlas
  uint32_t x, y;
  if (!findSpace(width, height, x, y)) {
    std::cerr << "Could not find space for glyph" << std::endl;
    return false;
  }

  // Verify bounds
  if (x + width > m_width || y + height > m_height) {
    std::cerr << "Glyph position exceeds atlas bounds" << std::endl;
    return false;
  }

  // Set active texture unit
  glActiveTexture(GL_TEXTURE0);
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cerr << "Error setting active texture: " << error << std::endl;
    return false;
  }

  // Bind texture
  glBindTexture(GL_TEXTURE_2D, m_textureId);
  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cerr << "Error binding texture in addGlyph: " << error << std::endl;
    return false;
  }

  // Set pixel storage mode
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cerr << "Error setting pixel storage mode: " << error << std::endl;
    return false;
  }

  // Add glyph to texture
  glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RED,
                  GL_UNSIGNED_BYTE, bitmap);
  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cerr << "Error in glTexSubImage2D: " << error << std::endl;
    return false;
  }

  // Store glyph info
  GlyphInfo info;
  info.u0 = static_cast<float>(x) / m_width;
  info.v0 = static_cast<float>(y) / m_height;
  info.u1 = static_cast<float>(x + width) / m_width;
  info.v1 = static_cast<float>(y + height) / m_height;
  info.bearingX = bearingX;
  info.bearingY = bearingY;
  info.advance = advance;
  info.width = width;
  info.height = height;

  m_glyphs[codepoint] = info;

  // Verify texture is still valid
  isTexture = glIsTexture(m_textureId);
  if (!isTexture) {
    std::cerr << "Texture became invalid after adding glyph" << std::endl;
    return false;
  }

  // Unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

const FontAtlas::GlyphInfo *FontAtlas::getGlyph(uint32_t codepoint) const {
  auto it = m_glyphs.find(codepoint);
  return it != m_glyphs.end() ? &it->second : nullptr;
}

FontAtlas::FontAtlas(FontAtlas &&other) noexcept {
  GLboolean isTexture = glIsTexture(other.m_textureId);

  m_textureId = other.m_textureId;
  m_width = other.m_width;
  m_height = other.m_height;
  m_currentX = other.m_currentX;
  m_currentY = other.m_currentY;
  m_rowHeight = other.m_rowHeight;
  m_glyphs = std::move(other.m_glyphs);

  other.m_textureId = 0;

  // Verify our texture is valid after move
  isTexture = glIsTexture(m_textureId);
}

FontAtlas &FontAtlas::operator=(FontAtlas &&other) noexcept {
  if (this != &other) {
    if (m_textureId != 0) {
      glDeleteTextures(1, &m_textureId);
    }

    GLboolean isTexture = glIsTexture(other.m_textureId);

    m_textureId = other.m_textureId;
    m_width = other.m_width;
    m_height = other.m_height;
    m_currentX = other.m_currentX;
    m_currentY = other.m_currentY;
    m_rowHeight = other.m_rowHeight;
    m_glyphs = std::move(other.m_glyphs);

    other.m_textureId = 0;

    // Verify our texture is valid after move
    isTexture = glIsTexture(m_textureId);
  }
  return *this;
}

std::optional<Font> Font::createFromFile(const std::string &path,
                                         CreateInfo &createInfo) {
  auto &library = FontLibrary::get();
  if (!library.isValid()) {
    createInfo.success = false;
    createInfo.errorMsg =
        "FreeType library not initialized: " + library.getError();
    return std::nullopt;
  }

  FT_Face face;
  if (FT_New_Face(library.getLibrary(), path.c_str(), 0, &face) != 0) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to load font: " + path;
    return std::nullopt;
  }

  if (FT_Set_Pixel_Sizes(face, 0, createInfo.size) != 0) {
    FT_Done_Face(face);
    createInfo.success = false;
    createInfo.errorMsg = "Failed to set font size";
    return std::nullopt;
  }

  return Font(face, createInfo.size);
}

Font::Font(FT_Face face, uint32_t size) : m_face(face) {
  // Check if texture is valid before any operations
  GLboolean isTexture = glIsTexture(m_atlas.getTexture());

  m_lineHeight = static_cast<float>(m_face->size->metrics.height >> 6);
  m_baseline = static_cast<float>(m_face->size->metrics.ascender >> 6);

  // Check after setup
  isTexture = glIsTexture(m_atlas.getTexture());
}

Font::~Font() {
  if (m_face) {
    FT_Done_Face(m_face);
  }
}

Font::Font(Font &&other) noexcept
    : m_face(other.m_face), m_atlas(std::move(other.m_atlas)),
      m_lineHeight(other.m_lineHeight), m_baseline(other.m_baseline) {
  other.m_face = nullptr;
}

Font &Font::operator=(Font &&other) noexcept {
  if (this != &other) {
    if (m_face) {
      FT_Done_Face(m_face);
    }
    m_face = other.m_face;
    m_atlas = std::move(other.m_atlas);
    m_lineHeight = other.m_lineHeight;
    m_baseline = other.m_baseline;
    other.m_face = nullptr;
  }
  return *this;
}

bool Font::cacheGlyph(uint32_t codepoint) {
  // Check if glyph is already cached
  if (getGlyphInfo(codepoint)) {
    return true;
  }

  // Load glyph
  if (FT_Load_Char(m_face, codepoint, FT_LOAD_RENDER) != 0) {
    return false;
  }

  // Get glyph bitmap
  FT_GlyphSlot glyph = m_face->glyph;
  return m_atlas.addGlyph(codepoint, glyph->bitmap.buffer, glyph->bitmap.width,
                          glyph->bitmap.rows, glyph->bitmap_left,
                          glyph->bitmap_top, glyph->advance.x >> 6);
}

float Font::getKerning(uint32_t first, uint32_t second) const {
  if (!FT_HAS_KERNING(m_face)) {
    return 0.0f;
  }

  FT_Vector kerning;
  FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, first),
                 FT_Get_Char_Index(m_face, second), FT_KERNING_DEFAULT,
                 &kerning);

  return static_cast<float>(kerning.x >> 6);
}

const FontAtlas::GlyphInfo *Font::getGlyphInfo(uint32_t codepoint) const {
  return m_atlas.getGlyph(codepoint);
}

TextRenderer::TextRenderer(std::shared_ptr<Renderer2D> renderer)
    : m_renderer(renderer) {}

TextRenderer::TextMetrics
TextRenderer::calculateMetrics(const Font &font,
                               const std::string &text) const {
  TextMetrics metrics{0.0f, font.getLineHeight(), font.getBaseline()};

  uint32_t prevChar = 0;
  for (char c : text) {
    uint32_t codepoint = static_cast<uint32_t>(c);
    if (const auto *glyph = font.getGlyphInfo(codepoint)) {
      metrics.width += glyph->advance + font.getKerning(prevChar, codepoint);
      prevChar = codepoint;
    }
  }

  return metrics;
}

void TextRenderer::renderText(Font &font, const std::string &text,
                              const glm::vec2 &position,
                              const glm::vec4 &color) {

  // Cache glyphs first
  for (char c : text) {
    uint32_t codepoint = static_cast<uint32_t>(c);
    if (!font.getGlyphInfo(codepoint)) {
      font.cacheGlyph(codepoint);
    }
  }

  // Store original blend state
  GLint blendSrc, blendDst;
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);

  // Set up text blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glm::vec2 pen = position;
  uint32_t prevChar = 0;
  uint32_t fontTexture = font.getAtlasTexture();

  // Create a TextureInfo for the font atlas
  Renderer2D::TextureInfo fontTextureInfo{
      fontTexture,
      0, // Width not needed for atlas
      0, // Height not needed for atlas
      0  // Will be assigned by drawTexturedQuad
  };

  for (char c : text) {
    uint32_t codepoint = static_cast<uint32_t>(c);
    const auto *glyph = font.getGlyphInfo(codepoint);
    if (!glyph)
      continue;

    // Apply kerning
    pen.x += font.getKerning(prevChar, codepoint);

    // Calculate glyph position
    float x = pen.x + glyph->bearingX;
    // Position relative to baseline
    float y = pen.y + (font.getBaseline() - glyph->bearingY);

    // Draw glyph
    m_renderer->drawTexturedQuad(
        {x + glyph->width * 0.5f, y + glyph->height * 0.5f, 0.0f},
        fontTextureInfo, {glyph->width, glyph->height}, color, 0.0f,
        {0.5f, 0.5f}, {glyph->u0, glyph->v0, glyph->u1, glyph->v1});

    // Advance pen and update previous character
    pen.x += glyph->advance;
    prevChar = codepoint;
  }

  // Restore original blend state
  glBlendFunc(blendSrc, blendDst);
}

Text TextRenderer::createText(Font &font, const std::string &text,
                              const glm::vec2 &position,
                              const glm::vec4 &color) {
  return Text(*this, font, text, position, color);
}

Text::Text(TextRenderer &renderer, Font &font, const std::string &text,
           const glm::vec2 &position, const glm::vec4 &color)
    : m_renderer(renderer), m_font(font), m_text(text), m_position(position),
      m_color(color) {}

void Text::render() {
  m_renderer.renderText(m_font, m_text, m_position, m_color);
}

glm::vec2 Text::getSize() const {
  TextRenderer::TextMetrics metrics =
      m_renderer.calculateMetrics(m_font, m_text);
  return {metrics.width, metrics.height};
}

} // namespace ste