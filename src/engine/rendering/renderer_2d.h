#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include <glm/glm.hpp>

#include "shader.h"

namespace ste {

class Renderer2D {
public:
  struct CreateInfo {
    std::string errorMsg;
    bool success = true;
  };

  struct TextureInfo {
    uint32_t id;
    int32_t width;
    int32_t height;
  };

  struct Statistics {
    uint32_t drawCalls = 0;
    uint32_t quadCount = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
  };

  static std::optional<Renderer2D> create(CreateInfo &createInfo);

  ~Renderer2D();
  Renderer2D(const Renderer2D &) = delete;
  Renderer2D &operator=(const Renderer2D &) = delete;
  Renderer2D(Renderer2D &&other) noexcept;
  Renderer2D &operator=(Renderer2D &&other) noexcept;

  // Begin/End rendering
  void beginScene(const glm::mat4 &viewProjection);
  void endScene();

  // Primitive rendering methods
  void drawQuad(const glm::vec2 &position, const glm::vec2 &size = {1.0f, 1.0f},
                const glm::vec4 &color = {1.0f, 1.0f, 1.0f, 1.0f},
                float rotation = 0.0f);

  void drawQuad(const glm::vec3 &position, const glm::vec2 &size = {1.0f, 1.0f},
                const glm::vec4 &color = {1.0f, 1.0f, 1.0f, 1.0f},
                float rotation = 0.0f);

  void drawTexturedQuad(const glm::vec2 &position, const TextureInfo &texture,
                        const glm::vec2 &size = {1.0f, 1.0f},
                        const glm::vec4 &tint = {1.0f, 1.0f, 1.0f, 1.0f},
                        float rotation = 0.0f,
                        const glm::vec4 &texCoords = {0.0f, 0.0f, 1.0f, 1.0f});

  void drawTexturedQuad(const glm::vec3 &position, const TextureInfo &texture,
                        const glm::vec2 &size = {1.0f, 1.0f},
                        const glm::vec4 &tint = {1.0f, 1.0f, 1.0f, 1.0f},
                        float rotation = 0.0f,
                        const glm::vec4 &texCoords = {0.0f, 0.0f, 1.0f, 1.0f});

  // Statistics for debugging/profiling
  void resetStats();
  Statistics getStats() const;

private:
  struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoords;
    float texIndex;
    float tilingFactor;
  };

  static constexpr uint32_t MAX_QUADS = 10000;
  static constexpr uint32_t MAX_VERTICES = MAX_QUADS * 4;
  static constexpr uint32_t MAX_INDICES = MAX_QUADS * 6;
  static constexpr uint32_t MAX_TEXTURE_SLOTS = 16;

  // New constants for ring buffer
  static constexpr uint32_t BUFFER_COUNT = 3; // Triple buffering
  static constexpr uint64_t MAX_SYNC_WAIT_NANOS =
      1000000000; // 1 second timeout

  Shader m_shader;
  uint32_t m_VAO{0};
  uint32_t m_VBO{0};
  uint32_t m_IBO{0};

  uint32_t m_indexCount{0};
  Vertex *m_vertexBufferBase{nullptr};
  Vertex *m_vertexBufferPtr{nullptr};

  glm::mat4 m_viewProjection{1.0f};
  Statistics m_stats{};

  uint32_t m_currentBuffer{0};
  GLsync m_fences[BUFFER_COUNT]{nullptr};
  uint32_t m_lastTextureId{0}; // Cache for texture binding optimization

  explicit Renderer2D(Shader &&shader, uint32_t vao, uint32_t vbo, uint32_t ibo,
                      Vertex *vertices);
  void flush();
  void startBatch();
  void waitForBuffer(uint32_t bufferIndex);
};

} // namespace ste