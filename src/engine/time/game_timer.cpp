#include "game_timer.h"

namespace ste {

GameTimer::GameTimer(int targetFPS)
    : targetFPS(targetFPS), targetFrameTime(1.0f / targetFPS),
      lastFrameTime(SDL_GetTicks64()), frameCount(0), fpsTimer(0.0f),
      currentFPS(0.0f), deltaTime(0.0f), pauseState(false), timeScale(1.0f),
      totalTime(0.0f), logFPSCounter(false) {}

void GameTimer::update() {
  if (pauseState) {
    deltaTime = 0.0f;
    lastFrameTime = SDL_GetTicks64();
    return;
  }

  float rawDeltaTime = calculateDeltaTime();
  deltaTime = rawDeltaTime * timeScale;

  capDeltaTime(deltaTime);
  totalTime += deltaTime;

  updateFPSCounter(rawDeltaTime);
}

float GameTimer::calculateDeltaTime() const {
  Uint64 currentTime = SDL_GetTicks64();
  return (currentTime - lastFrameTime) / 1000.0f;
}

void GameTimer::capDeltaTime(float &dt) const {
  if (dt > MAX_DELTA_TIME * timeScale) {
    dt = MAX_DELTA_TIME * timeScale;
  }
}

void GameTimer::updateFPSCounter(float unscaledDeltaTime) {
  frameCount++;
  fpsTimer += unscaledDeltaTime;

  if (fpsTimer >= FPS_UPDATE_INTERVAL && logFPSCounter) {
    currentFPS = frameCount / fpsTimer;
    frameCount = 0;
    fpsTimer = 0.0f;

    if (logFPSCounter) {
      logFPSStats();
    }
  }
}

void GameTimer::logFPSStats() const {
  std::cout << "FPS: " << currentFPS << " | Time Scale: " << timeScale
            << " | Total Time: " << totalTime
            << " | Paused: " << (pauseState ? "Yes" : "No") << std::endl;
}

void GameTimer::limitFrameRate() {
  if (!pauseState) {
    float frameTime = calculateDeltaTime();
    if (frameTime < targetFrameTime) {
      SDL_Delay(static_cast<Uint32>((targetFrameTime - frameTime) * 1000.0f));
    }
  }
}

void GameTimer::setPaused(bool paused) {
  if (pauseState != paused) {
    pauseState = paused;
    if (paused) {
      deltaTime = 0.0f;
    }
  }
}

void GameTimer::setTimeScale(float scale) {
  if (scale >= 0.0f) {
    timeScale = scale;
  }
}

} // namespace ste
