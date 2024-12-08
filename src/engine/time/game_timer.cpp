#include "game_timer.h"

namespace ste {

GameTimer::GameTimer(int targetFPS)
    : m_targetFPS(targetFPS), m_targetFrameTime(1.0f / targetFPS),
      m_lastFrameTime(SDL_GetTicks64()), m_frameCount(0), m_fpsTimer(0.0f),
      m_currentFPS(0.0f), m_deltaTime(0.0f), m_pauseState(false),
      m_timeScale(1.0f), m_totalTime(0.0f) {}

void GameTimer::update() {
  if (m_pauseState) {
    m_deltaTime = 0.0f;
    m_lastFrameTime = SDL_GetTicks64();
    return;
  }

  Uint64 currentTime = SDL_GetTicks64();
  float rawDeltaTime = (currentTime - m_lastFrameTime) / 1000.0f;
  m_lastFrameTime = currentTime;

  m_deltaTime = rawDeltaTime * m_timeScale;
  capDeltaTime(m_deltaTime);
  m_totalTime += m_deltaTime;

  updateFPSCounter(rawDeltaTime);
}

void GameTimer::updateFPSCounter(float unscaledDeltaTime) {
  m_frameCount++;
  m_fpsTimer += unscaledDeltaTime;

  if (m_fpsTimer >= FPS_UPDATE_INTERVAL) {
    // Calculate FPS based on the actual time elapsed
    m_currentFPS = static_cast<float>(m_frameCount) / m_fpsTimer;

    // Reset counters
    m_frameCount = 0;
    m_fpsTimer = 0.0f;
  }
}

void GameTimer::capDeltaTime(float &dt) const {
  if (dt > MAX_DELTA_TIME * m_timeScale) {
    dt = MAX_DELTA_TIME * m_timeScale;
  }
}

void GameTimer::limitFrameRate() {
  Uint64 currentTime = SDL_GetTicks64();
  float frameTime = (currentTime - m_lastFrameTime) / 1000.0f;

  if (frameTime < m_targetFrameTime) {
    SDL_Delay(static_cast<Uint32>((m_targetFrameTime - frameTime) * 1000.0f));
  }
}

float GameTimer::calculateDeltaTime() const {
  Uint64 currentTime = SDL_GetTicks64();
  return (currentTime - m_lastFrameTime) / 1000.0f;
}

void GameTimer::setPaused(bool paused) {
  if (m_pauseState != paused) {
    m_pauseState = paused;
    if (paused) {
      m_deltaTime = 0.0f;
    }
    m_lastFrameTime = SDL_GetTicks64();
  }
}

void GameTimer::setTimeScale(float scale) {
  if (scale >= 0.0f) {
    m_timeScale = scale;
  }
}

} // namespace ste