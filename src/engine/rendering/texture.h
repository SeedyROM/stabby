#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glad/glad.h>

namespace ste {

class Texture {
public:
  struct CreateInfo {
    std::string errorMsg;
    bool success = true;
    bool generateMipmaps = true;
    bool flipVertically = true;
    GLenum wrapS = GL_REPEAT;
    GLenum wrapT = GL_REPEAT;
    GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;
    GLenum magFilter = GL_LINEAR;
  };

  static std::optional<Texture> createFromFile(const std::string &path,
                                               CreateInfo &createInfo);
  static std::optional<Texture>
  createFromMemory(const uint8_t *data, size_t size, CreateInfo &createInfo);

  ~Texture();
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;
  Texture(Texture &&other) noexcept;
  Texture &operator=(Texture &&other) noexcept;

  void bind(uint32_t slot = 0) const;
  void unbind() const;

  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }
  uint32_t getId() const { return m_id; }

private:
  explicit Texture(uint32_t id, int width, int height);
  static std::optional<Texture> createFromSurface(SDL_Surface *surface,
                                                  CreateInfo &createInfo);

  uint32_t m_id{0};
  int m_width{0};
  int m_height{0};
};

} // namespace ste