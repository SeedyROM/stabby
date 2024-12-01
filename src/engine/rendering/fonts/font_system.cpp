#include "font_system.h"
#include "font.h"
#include <algorithm>
#include <cmath>

namespace ste {

namespace {
// Fast SDF generation using 8SSEDT algorithm
std::vector<uint8_t> generateSDF(const uint8_t *bitmap, uint32_t width,
                                 uint32_t height, uint32_t padding) {
  const uint32_t paddedWidth = width + 2 * padding;
  const uint32_t paddedHeight = height + 2 * padding;
  std::vector<uint8_t> sdf(paddedWidth * paddedHeight);

  // Initialize with max distance
  const float maxDist = std::sqrt(width * width + height * height);
  std::vector<float> dist(paddedWidth * paddedHeight, maxDist);

  // First pass: inside distances
  for (uint32_t y = padding; y < paddedHeight - padding; ++y) {
    for (uint32_t x = padding; x < paddedWidth - padding; ++x) {
      if (bitmap[(y - padding) * width + (x - padding)] > 127) {
        dist[y * paddedWidth + x] = 0;
      }
    }
  }

  // Two-pass sweep
  for (uint32_t y = 1; y < paddedHeight - 1; ++y) {
    // Forward pass
    for (uint32_t x = 1; x < paddedWidth - 1; ++x) {
      float &d = dist[y * paddedWidth + x];
      d = std::min({d, dist[y * paddedWidth + (x - 1)] + 1.0f,
                    dist[(y - 1) * paddedWidth + x] + 1.0f});
    }
    // Backward pass
    for (uint32_t x = paddedWidth - 2; x > 0; --x) {
      float &d = dist[y * paddedWidth + x];
      d = std::min(d, dist[y * paddedWidth + (x + 1)] + 1.0f);
    }
  }

  // Normalize and convert to unsigned byte
  const float scale = 1.0f / (padding * 2.0f);
  for (uint32_t i = 0; i < dist.size(); ++i) {
    float normalized = std::clamp(dist[i] * scale, 0.0f, 1.0f);
    sdf[i] = static_cast<uint8_t>(normalized * 255.0f);
  }

  return sdf;
}
} // namespace

std::optional<FontSystem> FontSystem::create(CreateInfo &createInfo) {
  FT_Library ftLibrary;
  if (FT_Init_FreeType(&ftLibrary)) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to initialize FreeType";
    return std::nullopt;
  }
  return FontSystem(ftLibrary);
}

FontSystem::FontSystem(FT_Library ftLibrary) : m_ftLibrary(ftLibrary) {}

FontSystem::~FontSystem() {
  if (m_ftLibrary) {
    FT_Done_FreeType(m_ftLibrary);
  }
}

FontSystem::FontSystem(FontSystem &&other) noexcept
    : m_ftLibrary(other.m_ftLibrary) {
  other.m_ftLibrary = nullptr;
}

FontSystem &FontSystem::operator=(FontSystem &&other) noexcept {
  if (this != &other) {
    if (m_ftLibrary) {
      FT_Done_FreeType(m_ftLibrary);
    }
    m_ftLibrary = other.m_ftLibrary;
    other.m_ftLibrary = nullptr;
  }
  return *this;
}

std::shared_ptr<Font> FontSystem::loadFont(const std::string &path,
                                           FontConfig &config) {
  FT_Face face;
  if (FT_New_Face(m_ftLibrary, path.c_str(), 0, &face)) {
    config.success = false;
    config.errorMsg = "Failed to load font: " + path;
    return nullptr;
  }

  // Set the font size
  FT_Set_Pixel_Sizes(face, 0, config.fontSize);

  // Create texture atlas
  const uint32_t atlasSize = config.atlasSize;
  std::vector<uint8_t> atlasBuffer(atlasSize * atlasSize, 0);
  std::unordered_map<char, GlyphMetrics> glyphs;

  // Track atlas packing position
  uint32_t x = config.padding;
  uint32_t y = config.padding;
  uint32_t rowHeight = 0;

  // Pack glyphs into atlas
  for (unsigned char c = 32; c < 128; c++) {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      continue;
    }

    // Generate SDF for this glyph
    auto sdf =
        generateSDF(face->glyph->bitmap.buffer, face->glyph->bitmap.width,
                    face->glyph->bitmap.rows, config.padding);

    const uint32_t glyphWidth = face->glyph->bitmap.width + 2 * config.padding;
    const uint32_t glyphHeight = face->glyph->bitmap.rows + 2 * config.padding;

    // Check if we need to advance to next row
    if (x + glyphWidth >= atlasSize) {
      x = config.padding;
      y += rowHeight + config.padding;
      rowHeight = 0;
    }

    // Check if we've exceeded atlas height
    if (y + glyphHeight >= atlasSize) {
      config.success = false;
      config.errorMsg = "Atlas size exceeded";
      FT_Done_Face(face);
      return nullptr;
    }

    // Copy SDF into atlas
    for (uint32_t py = 0; py < glyphHeight; ++py) {
      for (uint32_t px = 0; px < glyphWidth; ++px) {
        atlasBuffer[(y + py) * atlasSize + (x + px)] =
            sdf[py * glyphWidth + px];
      }
    }

    // Store glyph metrics
    GlyphMetrics metrics;
    metrics.size = {face->glyph->bitmap.width, face->glyph->bitmap.rows};
    metrics.bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top};
    metrics.advance = {face->glyph->advance.x >> 6,
                       face->glyph->advance.y >> 6};
    metrics.texCoords = {static_cast<float>(x) / atlasSize,
                         static_cast<float>(y) / atlasSize,
                         static_cast<float>(x + glyphWidth) / atlasSize,
                         static_cast<float>(y + glyphHeight) / atlasSize};
    metrics.scale = static_cast<float>(config.padding);

    glyphs[c] = metrics;

    // Advance atlas position
    x += glyphWidth + config.padding;
    rowHeight = std::max(rowHeight, glyphHeight);
  }

  // Create texture from atlas
  Texture::CreateInfo textureInfo;
  textureInfo.generateMipmaps = config.generateMips;
  textureInfo.minFilter =
      config.generateMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
  textureInfo.magFilter = GL_LINEAR;
  textureInfo.wrapS = GL_CLAMP_TO_EDGE;
  textureInfo.wrapT = GL_CLAMP_TO_EDGE;

  auto texture = Texture::createFromMemory(atlasBuffer.data(),
                                           atlasSize * atlasSize, textureInfo);
  if (!texture) {
    config.success = false;
    config.errorMsg = "Failed to create font texture atlas";
    FT_Done_Face(face);
    return nullptr;
  }

  float lineHeight = face->size->metrics.height >> 6;
  float baseline = face->size->metrics.ascender >> 6;

  FT_Done_Face(face);

  return std::make_shared<Font>(std::move(*texture), std::move(glyphs),
                                lineHeight, baseline, shared_from_this());
}

} // namespace ste