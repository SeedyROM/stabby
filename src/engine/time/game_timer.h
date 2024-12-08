#pragma once

#include <SDL2/SDL.h>
#include <iostream>

namespace ste {

class GameTimer {
public:
  GameTimer(int targetFPS = 60);

  // Public interface - what other code actually needs to know about
  void update();
  void limitFrameRate();

  // Getters - public interface for reading state
  float getDeltaTime() const { return m_deltaTime; }
  float getFPS() const { return m_currentFPS; }
  bool isPaused() const { return m_pauseState; }
  float getTimeScale() const { return m_timeScale; }
  float getTotalTime() const { return m_totalTime; }

  // State modification interface
  void setPaused(bool paused);
  void togglePause() { setPaused(!m_pauseState); }
  void setTimeScale(float scale);

  // Preset time modifications
  void setSlowMotion() { setTimeScale(0.5f); }
  void setNormalSpeed() { setTimeScale(1.0f); }
  void setFastForward() { setTimeScale(2.0f); }

private:
  // Internal helper methods
  void updateFPSCounter(float unscaledDeltaTime);
  void logFPSStats() const;
  float calculateDeltaTime() const;
  void capDeltaTime(float &dt) const;

  // Configuration constants
  const int m_targetFPS;
  const float m_targetFrameTime;

  // Time tracking
  Uint64 m_lastFrameTime;
  float m_deltaTime;
  float m_totalTime;

  // FPS tracking
  int m_frameCount;
  float m_fpsTimer;
  float m_currentFPS;

  // Time control
  bool m_pauseState;
  float m_timeScale;

  // Constants
  static constexpr float MAX_DELTA_TIME = 0.1f;
  static constexpr float FPS_UPDATE_INTERVAL = 0.1f;
};

} // namespace ste