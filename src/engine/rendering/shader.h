#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace ste {

class Shader {
public:
  enum class Error {
    None,
    FileNotFound,
    ReadError,
    CompilationFailed,
    LinkingFailed
  };

  struct CreateInfo {
    std::string errorMsg;
    Error error = Error::None;
  };

  static std::optional<Shader>
  createFromFilesystem(std::string_view vertexPath,
                       std::string_view fragmentPath, CreateInfo &createInfo);

  static std::optional<Shader> createFromMemory(std::string vertexSource,
                                                std::string fragmentSource,
                                                CreateInfo &createInfo);

  ~Shader();

  // Delete copy constructor and assignment operator
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;

  // Move constructor and assignment operator
  Shader(Shader &&other) noexcept;
  Shader &operator=(Shader &&other) noexcept;

  void use() const;

  // Uniform setters with template specialization for common types
  template <typename T>
  bool setUniform(std::string_view name, const T &value) const {
    const GLint location = getUniformLocation(name);
    if (location == -1)
      return false;

    if constexpr (std::is_same_v<T, bool>) {
      glUniform1i(location, static_cast<int>(value));
    } else if constexpr (std::is_same_v<T, int>) {
      glUniform1i(location, value);
    } else if constexpr (std::is_same_v<T, float>) {
      glUniform1f(location, value);
    } else if constexpr (std::is_same_v<T, glm::vec2>) {
      glUniform2fv(location, 1, &value[0]);
    } else if constexpr (std::is_same_v<T, glm::vec3>) {
      glUniform3fv(location, 1, &value[0]);
    } else if constexpr (std::is_same_v<T, glm::vec4>) {
      glUniform4fv(location, 1, &value[0]);
    } else if constexpr (std::is_same_v<T, glm::mat2>) {
      glUniformMatrix2fv(location, 1, GL_FALSE, &value[0][0]);
    } else if constexpr (std::is_same_v<T, glm::mat3>) {
      glUniformMatrix3fv(location, 1, GL_FALSE, &value[0][0]);
    } else if constexpr (std::is_same_v<T, glm::mat4>) {
      glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    } else {
      static_assert(!sizeof(T), "Unsupported uniform type");
      return false;
    }
    return true;
  }

  // Array uniform setter
  template <typename T>
  bool setUniformArray(std::string_view name, std::span<const T> values) const {
    const GLint location = getUniformLocation(name);
    if (location == -1)
      return false;

    if constexpr (std::is_same_v<T, int>) {
      glUniform1iv(location, values.size(), values.data());
    } else if constexpr (std::is_same_v<T, float>) {
      glUniform1fv(location, values.size(), values.data());
    } else if constexpr (std::is_same_v<T, glm::vec2>) {
      glUniform2fv(location, values.size(),
                   reinterpret_cast<const float *>(values.data()));
    } else if constexpr (std::is_same_v<T, glm::vec3>) {
      glUniform3fv(location, values.size(),
                   reinterpret_cast<const float *>(values.data()));
    } else if constexpr (std::is_same_v<T, glm::vec4>) {
      glUniform4fv(location, values.size(),
                   reinterpret_cast<const float *>(values.data()));
    } else {
      static_assert(!sizeof(T), "Unsupported uniform array type");
      return false;
    }
    return true;
  }

  template <size_t N>
  bool setUniformArray(std::string_view name, const int (&values)[N]) const {
    const GLint location = getUniformLocation(name);
    if (location == -1)
      return false;
    glUniform1iv(location, N, values);
    return true;
  }

private:
  GLuint m_id{0};
  mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;

  explicit Shader(GLuint id);

  static std::optional<std::string> readShaderFile(std::string_view filePath,
                                                   CreateInfo &createInfo);
  static bool compileShader(GLenum type, const char *source,
                            const char *typeStr, CreateInfo &createInfo,
                            GLuint &shaderId);
  static bool checkLinkErrors(GLuint programId, CreateInfo &createInfo);
  GLint getUniformLocation(std::string_view name) const;
};

} // namespace ste