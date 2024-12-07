#pragma once

#include <SDL2/SDL.h>
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <unordered_map>

namespace ste {

enum class Input {
  // Keyboard - Letters
  A = SDL_SCANCODE_A,
  B = SDL_SCANCODE_B,
  C = SDL_SCANCODE_C,
  D = SDL_SCANCODE_D,
  E = SDL_SCANCODE_E,
  F = SDL_SCANCODE_F,
  G = SDL_SCANCODE_G,
  H = SDL_SCANCODE_H,
  I = SDL_SCANCODE_I,
  J = SDL_SCANCODE_J,
  K = SDL_SCANCODE_K,
  L = SDL_SCANCODE_L,
  M = SDL_SCANCODE_M,
  N = SDL_SCANCODE_N,
  O = SDL_SCANCODE_O,
  P = SDL_SCANCODE_P,
  Q = SDL_SCANCODE_Q,
  R = SDL_SCANCODE_R,
  S = SDL_SCANCODE_S,
  T = SDL_SCANCODE_T,
  U = SDL_SCANCODE_U,
  V = SDL_SCANCODE_V,
  W = SDL_SCANCODE_W,
  X = SDL_SCANCODE_X,
  Y = SDL_SCANCODE_Y,
  Z = SDL_SCANCODE_Z,

  // Keyboard - Numbers
  Num0 = SDL_SCANCODE_0,
  Num1 = SDL_SCANCODE_1,
  Num2 = SDL_SCANCODE_2,
  Num3 = SDL_SCANCODE_3,
  Num4 = SDL_SCANCODE_4,
  Num5 = SDL_SCANCODE_5,
  Num6 = SDL_SCANCODE_6,
  Num7 = SDL_SCANCODE_7,
  Num8 = SDL_SCANCODE_8,
  Num9 = SDL_SCANCODE_9,

  // Keyboard - Function keys
  F1 = SDL_SCANCODE_F1,
  F2 = SDL_SCANCODE_F2,
  F3 = SDL_SCANCODE_F3,
  F4 = SDL_SCANCODE_F4,
  F5 = SDL_SCANCODE_F5,
  F6 = SDL_SCANCODE_F6,
  F7 = SDL_SCANCODE_F7,
  F8 = SDL_SCANCODE_F8,
  F9 = SDL_SCANCODE_F9,
  F10 = SDL_SCANCODE_F10,
  F11 = SDL_SCANCODE_F11,
  F12 = SDL_SCANCODE_F12,

  // Keyboard - Special keys
  Space = SDL_SCANCODE_SPACE,
  Enter = SDL_SCANCODE_RETURN,
  Tab = SDL_SCANCODE_TAB,
  Escape = SDL_SCANCODE_ESCAPE,
  Backspace = SDL_SCANCODE_BACKSPACE,
  Delete = SDL_SCANCODE_DELETE,
  LeftShift = SDL_SCANCODE_LSHIFT,
  RightShift = SDL_SCANCODE_RSHIFT,
  LeftControl = SDL_SCANCODE_LCTRL,
  RightControl = SDL_SCANCODE_RCTRL,
  LeftAlt = SDL_SCANCODE_LALT,
  RightAlt = SDL_SCANCODE_RALT,

  // Arrow keys
  Left = SDL_SCANCODE_LEFT,
  Right = SDL_SCANCODE_RIGHT,
  Up = SDL_SCANCODE_UP,
  Down = SDL_SCANCODE_DOWN,

  // Mouse buttons
  MouseLeft = SDL_BUTTON_LEFT - 1,
  MouseRight = SDL_BUTTON_RIGHT - 1,
  MouseMiddle = SDL_BUTTON_MIDDLE - 1,
  MouseX1 = SDL_BUTTON_X1 - 1,
  MouseX2 = SDL_BUTTON_X2 - 1
};

class InputManager {
public:
  enum class ButtonState { PRESSED, RELEASED, HELD };

  InputManager();
  ~InputManager() = default;

  // Delete copy operations
  InputManager(const InputManager &) = delete;
  InputManager &operator=(const InputManager &) = delete;

  // Core functionality
  void update();
  void reset();

  // Keyboard state using ste::Input
  ButtonState getKeyState(Input key) const;
  bool isKeyPressed(Input key) const;
  bool isKeyHeld(Input key) const;
  bool isKeyReleased(Input key) const;

  // Mouse state using ste::Input
  ButtonState getMouseButtonState(Input button) const;
  bool isMouseButtonPressed(Input button) const;
  bool isMouseButtonHeld(Input button) const;
  bool isMouseButtonReleased(Input button) const;

  // Mouse position
  glm::vec2 getMousePosition() const;
  glm::vec2 getMouseDelta() const;
  glm::vec2 getMouseWheel() const;
  void setMouseWheel(const glm::vec2 &wheel);

  // Screen/Window space conversion
  void setWindowSize(int width, int height);
  glm::vec2 screenToNDC(const glm::vec2 &screenPos) const;
  glm::vec2 NDCToScreen(const glm::vec2 &ndcPos) const;

private:
  static constexpr size_t MAX_KEYS = SDL_NUM_SCANCODES;
  static constexpr size_t MAX_MOUSE_BUTTONS = 5;

  // Keyboard state
  std::array<bool, MAX_KEYS> m_currentKeyStates;
  std::array<bool, MAX_KEYS> m_previousKeyStates;

  // Mouse button state
  std::array<bool, MAX_MOUSE_BUTTONS> m_currentMouseButtonStates;
  std::array<bool, MAX_MOUSE_BUTTONS> m_previousMouseButtonStates;

  // Mouse position
  glm::vec2 m_mousePosition;
  glm::vec2 m_previousMousePosition;
  glm::vec2 m_mouseDelta;
  glm::vec2 m_mouseWheel;

  // Window properties
  int m_windowWidth;
  int m_windowHeight;
};

} // namespace ste