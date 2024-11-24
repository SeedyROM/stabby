#pragma once

#include <SDL2/SDL.h>
#include <iostream>

namespace ste {

class GameTimer {
public:
  GameTimer(int targetFPS = 60);

  // Public interface - what other code actually needs to know about
  void update(); // Main update function (renamed from tick for clarity)
  void limitFrameRate();

  // Getters - public interface for reading state
  float getDeltaTime() const { return deltaTime; }
  float getFPS() const { return currentFPS; }
  bool isPaused() const { return pauseState; } // Renamed for clarity
  float getTimeScale() const { return timeScale; }
  float getTotalTime() const { return totalTime; }

  // State modification interface
  void setPaused(bool paused);
  void togglePause() { setPaused(!pauseState); }
  void setTimeScale(float scale);
  void setLogFPSStats(bool show) { logFPSCounter = show; }

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
  const int targetFPS;
  const float targetFrameTime; // Renamed for consistency

  // Time tracking
  Uint64 lastFrameTime; // Renamed for clarity
  float deltaTime;
  float totalTime;

  // FPS tracking
  int frameCount;
  float fpsTimer;
  float currentFPS;
  bool logFPSCounter; // Renamed for clarity

  // Time control
  bool pauseState; // Renamed for clarity
  float timeScale;

  // Constants
  static constexpr float MAX_DELTA_TIME = 0.1f;
  static constexpr float FPS_UPDATE_INTERVAL = 0.1f;
};

} // namespace ste