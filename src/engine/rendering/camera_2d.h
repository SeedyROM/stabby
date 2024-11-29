#pragma once

#include <glm/glm.hpp>

namespace ste {

class Camera2D {
public:
  Camera2D(float windowWidth, float windowHeight);

  // Position methods
  void setPosition(const glm::vec2 &position);
  void move(const glm::vec2 &offset);
  const glm::vec2 &getPosition() const;

  // Movement control
  void setVelocity(const glm::vec2 &velocity);
  void addVelocity(const glm::vec2 &velocity);
  const glm::vec2 &getVelocity() const;
  void setMaxVelocity(float maxVel);
  void setDamping(float damping);

  // Zoom methods
  void setZoom(float zoom);
  void addZoom(float delta);
  float getZoom() const;

  // Rotation methods (in radians)
  void setRotation(float rotation);
  void rotate(float delta);
  float getRotation() const;

  // Window size methods
  void setWindowSize(float width, float height);

  // Update
  void update(float deltaTime);

  // Matrix access
  const glm::mat4 &getViewProjectionMatrix();

  // Coordinate conversion
  glm::vec2 screenToWorld(const glm::vec2 &screenCoords) const;
  glm::vec2 worldToScreen(const glm::vec2 &worldCoords) const;

private:
  void updateViewMatrix();
  void updateProjectionMatrix();
  void clampVelocity();

private:
  glm::vec2 m_position; // Camera position
  glm::vec2 m_velocity; // Camera velocity
  float m_maxVelocity;  // Maximum velocity
  float m_damping;      // Velocity damping factor
  float m_zoom;         // Camera zoom level
  float m_rotation;     // Camera rotation in radians
  float m_windowWidth;  // Window width
  float m_windowHeight; // Window height

  glm::mat4 m_viewMatrix;
  glm::mat4 m_projectionMatrix;
  glm::mat4 m_viewProjectionMatrix;

  bool m_needsUpdate; // Flag to track if matrices need updating
};

}; // namespace ste