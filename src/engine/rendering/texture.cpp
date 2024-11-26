// texture.cpp
#include "texture.h"

namespace ste {

std::optional<Texture> Texture::createFromFile(const std::string &path,
                                               CreateInfo &createInfo) {
  // Initialize SDL_image if not already done
  static bool initialized = false;
  if (!initialized) {
    int flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(flags) & flags) != flags) {
      createInfo.success = false;
      createInfo.errorMsg = "Failed to initialize SDL_image: ";
      createInfo.errorMsg += IMG_GetError();
      return std::nullopt;
    }
    initialized = true;
  }

  SDL_Surface *surface = IMG_Load(path.c_str());
  if (!surface) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to load image: ";
    createInfo.errorMsg += IMG_GetError();
    return std::nullopt;
  }

  auto texture = createFromSurface(surface, createInfo);
  SDL_FreeSurface(surface);
  return texture;
}

std::optional<Texture> Texture::createFromMemory(const uint8_t *data,
                                                 size_t size,
                                                 CreateInfo &createInfo) {
  SDL_RWops *rw = SDL_RWFromConstMem(data, size);
  if (!rw) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to create RWops: ";
    createInfo.errorMsg += SDL_GetError();
    return std::nullopt;
  }

  SDL_Surface *surface = IMG_Load_RW(rw, 1); // 1 means auto-close
  if (!surface) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to load image from memory: ";
    createInfo.errorMsg += IMG_GetError();
    return std::nullopt;
  }

  return createFromSurface(surface, createInfo);
}

std::optional<Texture> Texture::createFromSurface(SDL_Surface *surface,
                                                  CreateInfo &createInfo) {
  // Generate OpenGL texture
  GLuint textureId;
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, createInfo.wrapS);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, createInfo.wrapT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, createInfo.minFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, createInfo.magFilter);

  // Determine format
  GLenum format;
  switch (surface->format->BytesPerPixel) {
  case 4:
    format = GL_RGBA;
    break;
  case 3:
    format = GL_RGB;
    break;
  case 1:
    format = GL_RED;
    break;
  default:
    glDeleteTextures(1, &textureId);
    createInfo.success = false;
    createInfo.errorMsg = "Unsupported image format";
    return std::nullopt;
  }

  // Upload texture data
  glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format,
               GL_UNSIGNED_BYTE, surface->pixels);

  // Generate mipmaps if requested
  if (createInfo.generateMipmaps) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  return Texture(textureId, surface->w, surface->h);
}

Texture::Texture(uint32_t id, int width, int height)
    : m_id(id), m_width(width), m_height(height) {}

Texture::~Texture() {
  if (m_id != 0) {
    glDeleteTextures(1, &m_id);
  }
}

Texture::Texture(Texture &&other) noexcept
    : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height) {
  other.m_id = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept {
  if (this != &other) {
    if (m_id != 0) {
      glDeleteTextures(1, &m_id);
    }
    m_id = other.m_id;
    m_width = other.m_width;
    m_height = other.m_height;
    other.m_id = 0;
  }
  return *this;
}

void Texture::bind(uint32_t slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

} // namespace ste