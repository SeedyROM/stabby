// input_manager.cpp
#include "input_manager.h"

namespace ste {

using ButtonState = InputManager::ButtonState;

InputManager::InputManager()
    : m_currentKeyStates{}, m_previousKeyStates{}, m_currentMouseButtonStates{},
      m_previousMouseButtonStates{}, m_mousePosition(0.0f),
      m_previousMousePosition(0.0f), m_mouseDelta(0.0f), m_mouseWheel(0.0f),
      m_windowWidth(0), m_windowHeight(0) {}

void InputManager::update() {
  // Store previous states
  m_previousKeyStates = m_currentKeyStates;
  m_previousMouseButtonStates = m_currentMouseButtonStates;
  m_previousMousePosition = m_mousePosition;

  // Update keyboard state
  const Uint8 *keyboardState = SDL_GetKeyboardState(nullptr);
  for (size_t i = 0; i < MAX_KEYS; ++i) {
    m_currentKeyStates[i] = keyboardState[i] != 0;
  }

  // Update mouse state
  int mouseX, mouseY;
  Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
  m_mousePosition =
      glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY));

  // Update mouse buttons
  m_currentMouseButtonStates[static_cast<size_t>(Input::MouseLeft)] =
      (mouseState & SDL_BUTTON_LMASK) != 0;
  m_currentMouseButtonStates[static_cast<size_t>(Input::MouseMiddle)] =
      (mouseState & SDL_BUTTON_MMASK) != 0;
  m_currentMouseButtonStates[static_cast<size_t>(Input::MouseRight)] =
      (mouseState & SDL_BUTTON_RMASK) != 0;
  m_currentMouseButtonStates[static_cast<size_t>(Input::MouseX1)] =
      (mouseState & SDL_BUTTON_X1MASK) != 0;
  m_currentMouseButtonStates[static_cast<size_t>(Input::MouseX2)] =
      (mouseState & SDL_BUTTON_X2MASK) != 0;

  // Update mouse delta
  m_mouseDelta = m_mousePosition - m_previousMousePosition;
}

void InputManager::reset() {
  m_currentKeyStates.fill(false);
  m_previousKeyStates.fill(false);
  m_currentMouseButtonStates.fill(false);
  m_previousMouseButtonStates.fill(false);
  m_mousePosition = glm::vec2(0.0f);
  m_previousMousePosition = glm::vec2(0.0f);
  m_mouseDelta = glm::vec2(0.0f);
  m_mouseWheel = glm::vec2(0.0f);
}

ButtonState InputManager::getKeyState(Input key) const {
  const auto scancode = static_cast<size_t>(key);
  if (scancode >= MAX_KEYS)
    return ButtonState::RELEASED;

  if (m_currentKeyStates[scancode]) {
    return m_previousKeyStates[scancode] ? ButtonState::HELD
                                         : ButtonState::PRESSED;
  }
  return m_previousKeyStates[scancode] ? ButtonState::RELEASED
                                       : ButtonState::RELEASED;
}

ButtonState InputManager::getMouseButtonState(Input button) const {
  const auto index = static_cast<size_t>(button);
  if (index >= MAX_MOUSE_BUTTONS)
    return ButtonState::RELEASED;

  if (m_currentMouseButtonStates[index]) {
    return m_previousMouseButtonStates[index] ? ButtonState::HELD
                                              : ButtonState::PRESSED;
  }
  return m_previousMouseButtonStates[index] ? ButtonState::RELEASED
                                            : ButtonState::RELEASED;
}

bool InputManager::isKeyPressed(Input key) const {
  return getKeyState(key) == ButtonState::PRESSED;
}

bool InputManager::isKeyHeld(Input key) const {
  return getKeyState(key) == ButtonState::HELD;
}

bool InputManager::isKeyReleased(Input key) const {
  return getKeyState(key) == ButtonState::RELEASED;
}

bool InputManager::isMouseButtonPressed(Input button) const {
  return getMouseButtonState(button) == ButtonState::PRESSED;
}

bool InputManager::isMouseButtonHeld(Input button) const {
  return getMouseButtonState(button) == ButtonState::HELD;
}

bool InputManager::isMouseButtonReleased(Input button) const {
  return getMouseButtonState(button) == ButtonState::RELEASED;
}

glm::vec2 InputManager::getMousePosition() const { return m_mousePosition; }

glm::vec2 InputManager::getMouseDelta() const { return m_mouseDelta; }

glm::vec2 InputManager::getMouseWheel() const { return m_mouseWheel; }

void InputManager::setMouseWheel(const glm::vec2 &wheel) {
  m_mouseWheel = wheel;
}

void InputManager::setWindowSize(int width, int height) {
  m_windowWidth = width;
  m_windowHeight = height;
}

glm::vec2 InputManager::screenToNDC(const glm::vec2 &screenPos) const {
  return glm::vec2((2.0f * screenPos.x) / m_windowWidth - 1.0f,
                   1.0f - (2.0f * screenPos.y) / m_windowHeight);
}

glm::vec2 InputManager::NDCToScreen(const glm::vec2 &ndcPos) const {
  return glm::vec2((ndcPos.x + 1.0f) * m_windowWidth * 0.5f,
                   (1.0f - ndcPos.y) * m_windowHeight * 0.5f);
}

} // namespace ste