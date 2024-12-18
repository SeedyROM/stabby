#include "camera_2d.h"

#include <glm/gtc/matrix_transform.hpp>

namespace ste {

Camera2D::Camera2D(float windowWidth, float windowHeight)
    : m_position(0.0f, 0.0f), m_velocity(0.0f, 0.0f), m_maxVelocity(1000.0f),
      m_damping(8.0f), m_zoom(1.0f), m_rotation(0.0f),
      m_windowWidth(windowWidth), m_windowHeight(windowHeight),
      m_needsUpdate(true) {
  updateViewMatrix();
  updateProjectionMatrix();
}

void Camera2D::setPosition(const glm::vec2 &position) {
  m_position = position;
  m_needsUpdate = true;
}

void Camera2D::move(const glm::vec2 &offset) {
  m_position += offset;
  m_needsUpdate = true;
}

const glm::vec2 &Camera2D::getPosition() const { return m_position; }

void Camera2D::setVelocity(const glm::vec2 &velocity) {
  m_velocity = velocity;
  clampVelocity();
}

void Camera2D::addVelocity(const glm::vec2 &velocity) {
  m_velocity += velocity;
  clampVelocity();
}

const glm::vec2 &Camera2D::getVelocity() const { return m_velocity; }

void Camera2D::setMaxVelocity(float maxVel) {
  m_maxVelocity = maxVel;
  clampVelocity();
}

void Camera2D::setDamping(float damping) { m_damping = damping; }

void Camera2D::clampVelocity() {
  float length = glm::length(m_velocity);
  if (length > m_maxVelocity) {
    m_velocity = glm::normalize(m_velocity) * m_maxVelocity;
  }
}

void Camera2D::update(float deltaTime) {
  // Update position based on velocity
  if (glm::length(m_velocity) > 0.01f) {
    m_position += m_velocity * deltaTime;
    m_needsUpdate = true;

    // Apply damping
    m_velocity *= (1.0f - (m_damping * deltaTime));
  }
}

void Camera2D::setZoom(float zoom) {
  float newZoom = glm::max(zoom, 0.1f); // Prevent negative or zero zoom
  if (m_zoom != newZoom) {
    // Get the world point at screen center
    glm::vec2 screenCenter =
        glm::vec2(m_windowWidth * 0.5f, m_windowHeight * 0.5f);
    glm::vec2 worldCenter = m_position + (screenCenter / m_zoom);

    // Update zoom
    m_zoom = newZoom;

    // Update position to keep the same world point at screen center
    m_position = worldCenter - (screenCenter / m_zoom);

    m_needsUpdate = true;
  }
}

void Camera2D::addZoom(float delta) { setZoom(m_zoom + delta); }

float Camera2D::getZoom() const { return m_zoom; }

void Camera2D::setRotation(float rotation) {
  m_rotation = rotation;
  m_needsUpdate = true;
}

void Camera2D::rotate(float delta) {
  m_rotation += delta;
  m_needsUpdate = true;
}

float Camera2D::getRotation() const { return m_rotation; }

void Camera2D::setWindowSize(float width, float height) {
  m_windowWidth = width;
  m_windowHeight = height;
  updateProjectionMatrix();
  m_needsUpdate = true;
}

const glm::mat4 &Camera2D::getViewProjectionMatrix() {
  if (m_needsUpdate) {
    updateViewMatrix();
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    m_needsUpdate = false;
  }
  return m_viewProjectionMatrix;
}

glm::vec2 Camera2D::screenToWorld(const glm::vec2 &screenCoords) const {
  // Convert screen coordinates to normalized device coordinates
  float normX = (2.0f * screenCoords.x) / m_windowWidth - 1.0f;
  float normY = 1.0f - (2.0f * screenCoords.y) / m_windowHeight;

  // Create homogeneous coordinate
  glm::vec4 clip = glm::vec4(normX, normY, 0.0f, 1.0f);

  // Transform to world space
  glm::mat4 invVP = glm::inverse(m_viewProjectionMatrix);
  glm::vec4 world = invVP * clip;

  return glm::vec2(world.x, world.y);
}

glm::vec2 Camera2D::worldToScreen(const glm::vec2 &worldCoords) const {
  // Transform world coordinates to clip space
  glm::vec4 clip = m_viewProjectionMatrix *
                   glm::vec4(worldCoords.x, worldCoords.y, 0.0f, 1.0f);

  // Perform perspective divide
  glm::vec3 ndc = glm::vec3(clip) / clip.w;

  // Convert to screen coordinates
  return glm::vec2((ndc.x + 1.0f) * m_windowWidth * 0.5f,
                   (1.0f - ndc.y) * m_windowHeight * 0.5f);
}

void Camera2D::updateViewMatrix() {
  m_viewMatrix = glm::mat4(1.0f);

  // Apply transformations in reverse order:

  // First translate to center of screen
  m_viewMatrix =
      glm::translate(m_viewMatrix, glm::vec3(m_windowWidth * 0.5f,
                                             m_windowHeight * 0.5f, 0.0f));

  // Then apply zoom
  m_viewMatrix = glm::scale(m_viewMatrix, glm::vec3(m_zoom, m_zoom, 1.0f));

  // Then apply rotation
  m_viewMatrix =
      glm::rotate(m_viewMatrix, m_rotation, glm::vec3(0.0f, 0.0f, 1.0f));

  // Finally translate back by position
  m_viewMatrix = glm::translate(
      m_viewMatrix,
      glm::vec3(-m_position.x - m_windowWidth * 0.5f / m_zoom,
                -m_position.y - m_windowHeight * 0.5f / m_zoom, 0.0f));
}

void Camera2D::updateProjectionMatrix() {
  m_projectionMatrix = glm::ortho(0.0f,           // Left
                                  m_windowWidth,  // Right
                                  m_windowHeight, // Top (swapped with bottom)
                                  0.0f,           // Bottom (swapped with top)
                                  -1.0f,          // Near
                                  1.0f            // Far
  );
}

}; // namespace ste