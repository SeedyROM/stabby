#include "renderer_2d.h"

#include <array>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace ste {

namespace {
const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 a_Position;
        layout (location = 1) in vec4 a_Color;
        layout (location = 2) in vec2 a_TexCoord;
        layout (location = 3) in float a_TexIndex;
        layout (location = 4) in float a_TilingFactor;
        layout (location = 5) in float a_OutlineThickness;
        layout (location = 6) in vec4 a_OutlineColor;

        uniform mat4 u_ViewProjection;

        out vec4 v_Color;
        out vec2 v_TexCoord;
        out float v_TexIndex;
        out float v_TilingFactor;
        out float v_OutlineThickness;
        out vec4 v_OutlineColor;

        void main() {
            v_Color = a_Color;
            v_TexCoord = a_TexCoord;
            v_TexIndex = a_TexIndex;
            v_TilingFactor = a_TilingFactor;
            v_OutlineThickness = a_OutlineThickness;
            v_OutlineColor = a_OutlineColor;
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";

const char *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec4 v_Color;
        in vec2 v_TexCoord;
        in float v_TexIndex;
        in float v_TilingFactor;
        in float v_OutlineThickness;
        in vec4 v_OutlineColor;

        uniform sampler2D u_Textures[16];

        void main() {
            vec4 texColor = v_Color;
            
            if (v_TexIndex > -0.5) {
                int index = int(v_TexIndex);
                texColor *= texture(u_Textures[index], v_TexCoord * v_TilingFactor);
            }
            
            // Calculate distance from edge
            vec2 distFromCenter = abs(v_TexCoord - 0.5) * 2.0;
            float maxDist = max(distFromCenter.x, distFromCenter.y);
            
            // Define outline edge
            float outlineEdge = 1.0 - v_OutlineThickness;
            
            if (maxDist > outlineEdge && v_OutlineThickness > 0.0) {
                FragColor = v_OutlineColor;
            } else {
                FragColor = texColor;
            }
        }
    )";
} // namespace

std::optional<Renderer2D> Renderer2D::create(CreateInfo &createInfo) {
  // Create shader
  Shader::CreateInfo shaderInfo;
  auto shader = Shader::createFromMemory(vertexShaderSource,
                                         fragmentShaderSource, shaderInfo);
  if (!shader) {
    createInfo.success = false;
    createInfo.errorMsg = std::move(shaderInfo.errorMsg);
    return std::nullopt;
  }

  // Enable alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  // Create vertex array
  uint32_t vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Create vertex buffer
  uint32_t vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex) * BUFFER_COUNT,
               nullptr, GL_DYNAMIC_DRAW);

  // Vertex attributes
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, position));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, color));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, texCoords));

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, texIndex));

  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, tilingFactor));

  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, outlineThickness));

  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const void *)offsetof(Vertex, outlineColor));

  // Create and set up index buffer
  uint32_t ibo;
  glGenBuffers(1, &ibo);

  std::vector<uint32_t> indices(MAX_INDICES);
  uint32_t offset = 0;
  for (uint32_t i = 0; i < MAX_INDICES; i += 6) {
    indices[i + 0] = offset + 0;
    indices[i + 1] = offset + 1;
    indices[i + 2] = offset + 2;
    indices[i + 3] = offset + 2;
    indices[i + 4] = offset + 3;
    indices[i + 5] = offset + 0;
    offset += 4;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
               indices.data(), GL_STATIC_DRAW);

  // Allocate vertex buffer memory
  auto *vertices = new Vertex[MAX_VERTICES];

  return Renderer2D(std::move(*shader), vao, vbo, ibo, vertices);
}

Renderer2D::Renderer2D(Shader &&shader, uint32_t vao, uint32_t vbo,
                       uint32_t ibo, Vertex *vertices)
    : m_shader(std::move(shader)), m_VAO(vao), m_VBO(vbo), m_IBO(ibo),
      m_vertexBufferBase(vertices) {
  m_vertexBufferPtr = m_vertexBufferBase;
}

Renderer2D::~Renderer2D() {
  // Clean up sync objects
  for (uint32_t i = 0; i < BUFFER_COUNT; ++i) {
    if (m_fences[i]) {
      glDeleteSync(m_fences[i]);
    }
  }

  glDeleteVertexArrays(1, &m_VAO);
  glDeleteBuffers(1, &m_VBO);
  glDeleteBuffers(1, &m_IBO);
  delete[] m_vertexBufferBase;
}

Renderer2D::Renderer2D(Renderer2D &&other) noexcept
    : m_shader(std::move(other.m_shader)), m_VAO(other.m_VAO),
      m_VBO(other.m_VBO), m_IBO(other.m_IBO), m_indexCount(other.m_indexCount),
      m_vertexBufferBase(other.m_vertexBufferBase),
      m_vertexBufferPtr(other.m_vertexBufferPtr),
      m_viewProjection(other.m_viewProjection), m_stats(other.m_stats) {
  other.m_VAO = 0;
  other.m_VBO = 0;
  other.m_IBO = 0;
  other.m_vertexBufferBase = nullptr;
  other.m_vertexBufferPtr = nullptr;
}

Renderer2D &Renderer2D::operator=(Renderer2D &&other) noexcept {
  if (this != &other) {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_IBO);
    delete[] m_vertexBufferBase;

    m_shader = std::move(other.m_shader);
    m_VAO = other.m_VAO;
    m_VBO = other.m_VBO;
    m_IBO = other.m_IBO;
    m_indexCount = other.m_indexCount;
    m_vertexBufferBase = other.m_vertexBufferBase;
    m_vertexBufferPtr = other.m_vertexBufferPtr;
    m_viewProjection = other.m_viewProjection;
    m_stats = other.m_stats;

    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_IBO = 0;
    other.m_vertexBufferBase = nullptr;
    other.m_vertexBufferPtr = nullptr;
  }
  return *this;
}

void Renderer2D::beginScene(const glm::mat4 &viewProjection) {
  m_viewProjection = viewProjection;
  startBatch();
  setBlendMode(BlendMode::Alpha);
}

void Renderer2D::endScene() { flush(); }

void Renderer2D::waitForBuffer(uint32_t bufferIndex) {
  if (m_fences[bufferIndex]) {
    // Wait for the fence with a timeout
    GLenum result;
    do {
      result =
          glClientWaitSync(m_fences[bufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT,
                           MAX_SYNC_WAIT_NANOS);
    } while (result == GL_TIMEOUT_EXPIRED);

    glDeleteSync(m_fences[bufferIndex]);
    m_fences[bufferIndex] = nullptr;
  }
}

void Renderer2D::startBatch() {
  m_indexCount = 0;
  m_vertexBufferPtr = m_vertexBufferBase;
  m_lastTextureId = 0; // Reset texture cache
}

void Renderer2D::flush() {
  if (m_indexCount == 0) {
    return;
  }

  // Wait for the current buffer to be available
  waitForBuffer(m_currentBuffer);

  // Calculate data size and offset
  uint32_t dataSize =
      static_cast<uint32_t>(reinterpret_cast<uint8_t *>(m_vertexBufferPtr) -
                            reinterpret_cast<uint8_t *>(m_vertexBufferBase));
  uint32_t bufferOffset = m_currentBuffer * MAX_VERTICES * sizeof(Vertex);

  // Update buffer data
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferSubData(GL_ARRAY_BUFFER, bufferOffset, dataSize, m_vertexBufferBase);

  // Update vertex attribute pointers for current buffer
  const void *baseOffset = reinterpret_cast<const void *>(bufferOffset);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<const void *>(
                            bufferOffset + offsetof(Vertex, position)));
  glVertexAttribPointer(
      1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
      reinterpret_cast<const void *>(bufferOffset + offsetof(Vertex, color)));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<const void *>(
                            bufferOffset + offsetof(Vertex, texCoords)));
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<const void *>(
                            bufferOffset + offsetof(Vertex, texIndex)));
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<const void *>(
                            bufferOffset + offsetof(Vertex, tilingFactor)));

  // Draw
  m_shader.use();
  m_shader.setUniform("u_ViewProjection", m_viewProjection);

  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

  // Place fence for this buffer
  m_fences[m_currentBuffer] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

  // Move to next buffer
  m_currentBuffer = (m_currentBuffer + 1) % BUFFER_COUNT;

  m_stats.drawCalls++;
}

void Renderer2D::drawQuad(const glm::vec3 &position, const glm::vec2 &size,
                          const glm::vec4 &color, float rotation,
                          float outlineThickness,
                          const glm::vec4 &outlineColor) {
  if (m_indexCount >= MAX_INDICES) {
    flush();
    startBatch();
  }

  glm::mat4 transform =
      glm::translate(glm::mat4(1.0f), position) *
      glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f}) *
      glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

  // Add vertices to buffer
  m_vertexBufferPtr->position = transform * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f);
  m_vertexBufferPtr->color = color;
  m_vertexBufferPtr->texCoords = {0.0f, 0.0f};
  m_vertexBufferPtr->texIndex = -1.0f;
  m_vertexBufferPtr->tilingFactor = 1.0f;
  m_vertexBufferPtr->outlineThickness = outlineThickness;
  m_vertexBufferPtr->outlineColor = outlineColor;
  m_vertexBufferPtr++;

  m_vertexBufferPtr->position = transform * glm::vec4(0.5f, -0.5f, 0.0f, 1.0f);
  m_vertexBufferPtr->color = color;
  m_vertexBufferPtr->texCoords = {1.0f, 0.0f};
  m_vertexBufferPtr->texIndex = -1.0f;
  m_vertexBufferPtr->tilingFactor = 1.0f;
  m_vertexBufferPtr->outlineThickness = outlineThickness;
  m_vertexBufferPtr->outlineColor = outlineColor;
  m_vertexBufferPtr++;

  m_vertexBufferPtr->position = transform * glm::vec4(0.5f, 0.5f, 0.0f, 1.0f);
  m_vertexBufferPtr->color = color;
  m_vertexBufferPtr->texCoords = {1.0f, 1.0f};
  m_vertexBufferPtr->texIndex = -1.0f;
  m_vertexBufferPtr->tilingFactor = 1.0f;
  m_vertexBufferPtr->outlineThickness = outlineThickness;
  m_vertexBufferPtr->outlineColor = outlineColor;
  m_vertexBufferPtr++;

  m_vertexBufferPtr->position = transform * glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f);
  m_vertexBufferPtr->color = color;
  m_vertexBufferPtr->texCoords = {0.0f, 1.0f};
  m_vertexBufferPtr->texIndex = -1.0f;
  m_vertexBufferPtr->tilingFactor = 1.0f;
  m_vertexBufferPtr->outlineThickness = outlineThickness;
  m_vertexBufferPtr->outlineColor = outlineColor;
  m_vertexBufferPtr++;

  m_indexCount += 6;
  m_stats.quadCount++;
  m_stats.vertexCount += 4;
  m_stats.indexCount += 6;
}

void Renderer2D::drawTexturedQuad(const glm::vec2 &position,
                                  const TextureInfo &texture,
                                  const glm::vec2 &size, const glm::vec4 &tint,
                                  float rotation, const glm::vec4 &texCoords) {
  drawTexturedQuad({position.x, position.y, 0.0f}, texture, size, tint,
                   rotation, texCoords);
}

void Renderer2D::drawTexturedQuad(const glm::vec3 &position,
                                  const TextureInfo &texture,
                                  const glm::vec2 &size, const glm::vec4 &tint,
                                  float rotation, const glm::vec4 &texCoords) {
  if (m_indexCount >= MAX_INDICES) {
    flush();
    startBatch();
  }

  // Pre-calculate sin/cos for rotation
  const float s = rotation != 0.0f ? std::sin(rotation) : 0.0f;
  const float c = rotation != 0.0f ? std::cos(rotation) : 1.0f;

  // Pre-calculate scaled dimensions
  const float halfWidth = size.x * 0.5f;
  const float halfHeight = size.y * 0.5f;

  // Pre-calculate vertex positions relative to center
  const glm::vec3 vertices[4] = {
      {-halfWidth, -halfHeight, 0.0f}, // Bottom left
      {halfWidth, -halfHeight, 0.0f},  // Bottom right
      {halfWidth, halfHeight, 0.0f},   // Top right
      {-halfWidth, halfHeight, 0.0f}   // Top left
  };

  // Pre-calculate texture coordinates
  const glm::vec2 uvs[4] = {
      {texCoords.x, texCoords.y}, // Bottom left
      {texCoords.z, texCoords.y}, // Bottom right
      {texCoords.z, texCoords.w}, // Top right
      {texCoords.x, texCoords.w}  // Top left
  };

  // Bind texture only once per batch if possible
  static uint32_t lastTextureId = 0;
  if (texture.id != lastTextureId) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    lastTextureId = texture.id;
  }

  // SIMD-friendly vertex processing
  for (int i = 0; i < 4; i++) {
    // Apply rotation and translation
    float x = vertices[i].x;
    float y = vertices[i].y;

    m_vertexBufferPtr->position = {position.x + (x * c - y * s),
                                   position.y + (x * s + y * c),
                                   position.z + vertices[i].z};

    // Reuse existing values where possible
    m_vertexBufferPtr->color = tint;
    m_vertexBufferPtr->texCoords = uvs[i];
    m_vertexBufferPtr->texIndex = 0.0f; // Always use texture slot 0
    m_vertexBufferPtr->tilingFactor = 1.0f;

    m_vertexBufferPtr++;
  }

  m_indexCount += 6;
  m_stats.quadCount++;
  m_stats.vertexCount += 4;
  m_stats.indexCount += 6;
}

void Renderer2D::resetStats() { m_stats = Statistics(); }

Renderer2D::Statistics Renderer2D::getStats() const { return m_stats; }

void Renderer2D::setBlendMode(BlendMode mode) {
  if (m_currentBlendMode != mode) {
    m_currentBlendMode = mode;
    applyBlendMode(mode);
  }
}

void Renderer2D::applyBlendMode(BlendMode mode) {
  switch (mode) {
  case BlendMode::None:
    glDisable(GL_BLEND);
    break;

  case BlendMode::Alpha:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case BlendMode::Additive:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case BlendMode::Multiply:
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case BlendMode::Screen:
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
    glBlendEquation(GL_FUNC_ADD);
    break;

  case BlendMode::Subtract:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    break;
  }
}

} // namespace ste