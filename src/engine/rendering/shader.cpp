#include "shader.h"

#include <fstream>
#include <sstream>

namespace ste {

std::optional<Shader>
Shader::createFromFilesystem(std::string_view vertexPath,
                             std::string_view fragmentPath,
                             CreateInfo &createInfo) {
  auto vertexCode = readShaderFile(vertexPath, createInfo);
  if (!vertexCode) {
    return std::nullopt;
  }

  auto fragmentCode = readShaderFile(fragmentPath, createInfo);
  if (!fragmentCode) {
    return std::nullopt;
  }

  return createFromMemory(*vertexCode, *fragmentCode, createInfo);
}

std::optional<Shader> Shader::createFromMemory(std::string vertexSource,
                                               std::string fragmentSource,
                                               CreateInfo &createInfo) {

  const char *vShaderCode = vertexSource.c_str();
  const char *fShaderCode = fragmentSource.c_str();

  // Compile shaders
  GLuint vertex;
  if (!compileShader(GL_VERTEX_SHADER, vShaderCode, "VERTEX", createInfo,
                     vertex)) {
    return std::nullopt;
  }

  GLuint fragment;
  if (!compileShader(GL_FRAGMENT_SHADER, fShaderCode, "FRAGMENT", createInfo,
                     fragment)) {
    glDeleteShader(vertex);
    return std::nullopt;
  }

  // Create and link program
  GLuint programId = glCreateProgram();
  glAttachShader(programId, vertex);
  glAttachShader(programId, fragment);
  glLinkProgram(programId);

  if (!checkLinkErrors(programId, createInfo)) {
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(programId);
    return std::nullopt;
  }

  // Delete shaders as they're linked into our program
  glDeleteShader(vertex);
  glDeleteShader(fragment);

  return Shader(programId);
}

Shader::Shader(GLuint id) : m_id(id) {}

Shader::~Shader() {
  if (m_id != 0) {
    glDeleteProgram(m_id);
  }
}

Shader::Shader(Shader &&other) noexcept : m_id(other.m_id) { other.m_id = 0; }

Shader &Shader::operator=(Shader &&other) noexcept {
  if (this != &other) {
    glDeleteProgram(m_id);
    m_id = other.m_id;
    other.m_id = 0;
  }
  return *this;
}

void Shader::use() const { glUseProgram(m_id); }

std::optional<std::string> Shader::readShaderFile(std::string_view filePath,
                                                  CreateInfo &createInfo) {
  std::ifstream shaderFile{
      std::string(filePath)}; // Convert string_view to string here
  if (!shaderFile) {
    createInfo.error = Error::FileNotFound;
    createInfo.errorMsg = "Failed to open shader file: ";
    createInfo.errorMsg += filePath;
    return std::nullopt;
  }

  try {
    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    return shaderStream.str();
  } catch (...) {
    createInfo.error = Error::ReadError;
    createInfo.errorMsg = "Error reading shader file: ";
    createInfo.errorMsg += filePath;
    return std::nullopt;
  }
}

bool Shader::compileShader(GLenum type, const char *source, const char *typeStr,
                           CreateInfo &createInfo, GLuint &shaderId) {
  shaderId = glCreateShader(type);
  glShaderSource(shaderId, 1, &source, nullptr);
  glCompileShader(shaderId);

  GLint success;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLchar infoLog[1024];
    glGetShaderInfoLog(shaderId, 1024, nullptr, infoLog);
    createInfo.error = Error::CompilationFailed;
    createInfo.errorMsg = "Shader compilation error of type ";
    createInfo.errorMsg += typeStr;
    createInfo.errorMsg += ": ";
    createInfo.errorMsg += infoLog;
    glDeleteShader(shaderId);
    return false;
  }
  return true;
}

bool Shader::checkLinkErrors(GLuint programId, CreateInfo &createInfo) {
  GLint success;
  glGetProgramiv(programId, GL_LINK_STATUS, &success);
  if (!success) {
    GLchar infoLog[1024];
    glGetProgramInfoLog(programId, 1024, nullptr, infoLog);
    createInfo.error = Error::LinkingFailed;
    createInfo.errorMsg = "Program linking error: ";
    createInfo.errorMsg += infoLog;
    return false;
  }
  return true;
}

GLint Shader::getUniformLocation(std::string_view name) const {
  std::string nameStr(name);
  auto it = m_uniformLocationCache.find(nameStr);
  if (it != m_uniformLocationCache.end()) {
    return it->second;
  }

  GLint location = glGetUniformLocation(m_id, nameStr.c_str());
  m_uniformLocationCache[nameStr] = location;
  return location;
}

} // namespace ste