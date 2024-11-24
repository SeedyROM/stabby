#pragma once

#include <SDL2/SDL.h>
#include <iostream>

class GameTimer {
public:
  GameTimer(int targetFPS = 60)
      : TARGET_FPS(targetFPS), TARGET_FRAME_TIME(1.0f / targetFPS),
        previousTime(SDL_GetTicks64()), frameCount(0), fpsTimer(0.0f),
        currentFPS(0.0f), deltaTime(0.0f), isPaused(false), timeScale(1.0f),
        totalTime(0.0f), showFPS(true) {}

  void tick() {
    if (isPaused) {
      deltaTime = 0.0f;
      previousTime = SDL_GetTicks64(); // Update previous time to prevent huge
                                       // delta after unpause
      return;
    }

    Uint64 currentTime = SDL_GetTicks64();
    deltaTime = (currentTime - previousTime) / 1000.0f * timeScale;

    // Cap the delta time to prevent large jumps
    if (deltaTime > 0.1f * timeScale) {
      deltaTime = 0.1f * timeScale;
    }

    previousTime = currentTime;
    totalTime += deltaTime;

    // Update FPS counter
    frameCount++;
    fpsTimer += deltaTime / timeScale; // Use unscaled time for FPS calculation
    if (fpsTimer >= 1.0f && showFPS) {
      currentFPS = frameCount / fpsTimer;
      frameCount = 0;
      fpsTimer = 0.0f;
      std::cout << "FPS: " << currentFPS << " | Time Scale: " << timeScale
                << " | Total Time: " << totalTime
                << " | Paused: " << (isPaused ? "Yes" : "No") << std::endl;
    }
  }

  void limitFrameRate() {
    if (!isPaused) { // Only limit frame rate when not paused
      float frameTime = (SDL_GetTicks64() - previousTime) / 1000.0f;
      if (frameTime < TARGET_FRAME_TIME) {
        SDL_Delay((Uint32)((TARGET_FRAME_TIME - frameTime) * 1000.0f));
      }
    }
  }

  // Getters
  float getDeltaTime() const { return deltaTime; }
  float getFPS() const { return currentFPS; }
  bool getPaused() const { return isPaused; }
  float getTimeScale() const { return timeScale; }
  float getTotalTime() const { return totalTime; }

  // Setters
  void setPaused(bool paused) { isPaused = paused; }
  void togglePause() { isPaused = !isPaused; }

  void setTimeScale(float scale) {
    if (scale >= 0.0f) { // Prevent negative time scale
      timeScale = scale;
    }
  }

  void setShowFPS(bool show) { showFPS = show; }

  // Time scale helpers
  void slowMotion() { timeScale = 0.5f; }
  void normalSpeed() { timeScale = 1.0f; }
  void fastForward() { timeScale = 2.0f; }

private:
  const int TARGET_FPS;
  const float TARGET_FRAME_TIME;
  Uint64 previousTime;
  int frameCount;
  float fpsTimer;
  float currentFPS;
  float deltaTime;
  bool isPaused;
  float timeScale;
  float totalTime;
  bool showFPS;
};