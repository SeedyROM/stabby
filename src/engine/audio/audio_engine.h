#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "audio_file.h"
#include "audio_queue.h"
#include "engine/assets/asset_loader.h"

namespace ste {

class AudioChannel {
public:
  AudioChannel() = default;
  ~AudioChannel() = default;

  // Core audio functions
  void update(float deltaTime);
  void mix(float *buffer, size_t frames);
  void play(AudioFile *file, float vol = 1.0f);
  void stop();

  // State control
  [[nodiscard]] bool isActive() const {
    return m_active && m_currentFile != nullptr;
  }
  void setVolume(float vol);
  void setPitch(float newPitch);
  void setPosition(float x, float y);
  void fadeVolume(float target, float duration);

  // Playback speed control
  void setTargetPlaybackSpeed(float speed);
  void setPlaybackSpeed(float speed);

private:
  // Audio data
  AudioFile *m_currentFile = nullptr;
  size_t m_position = 0;

  // Volume control
  float m_volume = 1.0f;
  float m_targetVolume = 1.0f;
  float m_fadeTimeRemaining = 0.0f;
  float m_fadeDuration = 0.0f;

  // Pitch and position
  float m_pitch = 1.0f;
  float m_positionX = 0.0f;
  float m_positionY = 0.0f;

  // State flags
  bool m_active = false;

  // Speed transition
  float m_currentSpeed = 1.0f;
  float m_targetSpeed = 1.0f;

  // Utility functions
  [[nodiscard]] float calculatePanLeft() const;
  [[nodiscard]] float calculatePanRight() const;
  void applySpatialization(float &left, float &right) const;
  [[nodiscard]] float interpolateSample(float position) const;
};

class AudioEngine {
public:
  AudioEngine();
  ~AudioEngine();

  // Deleted copy/move operations
  AudioEngine(const AudioEngine &) = delete;
  AudioEngine &operator=(const AudioEngine &) = delete;
  AudioEngine(AudioEngine &&) = delete;
  AudioEngine &operator=(AudioEngine &&) = delete;

  // Main interface
  void playSound(AssetHandle<AudioFile> sound, float volume = 1.0f);
  void playMusic(AssetHandle<AudioFile> music, bool loop = true);
  void stopChannel(int channelId);
  void stopAll();
  void setChannelVolume(int channelId, float volume);
  void setChannelPitch(int channelId, float pitch);
  void setChannelPosition(int channelId, float x, float y);
  void fadeChannel(int channelId, float targetVolume, float duration);
  void setMasterVolume(float volume);

  // TODO(SeedyROM): Move this to a separate file
  void beginShutdown() {
    m_shutdownRequested = true;
    m_shutdownRampRemaining = SHUTDOWN_RAMP_DURATION;
  }
  void setSpeed(float newSpeed) {
    m_gameSpeed = std::clamp(newSpeed, 0.1f, 3.0f);
  }

  float getSpeed() const { return m_gameSpeed; }

  // Audio thread callback
  void audioCallback(float *buffer, size_t frames);

private:
  static constexpr size_t MAX_CHANNELS = 16;
  static constexpr float SHUTDOWN_RAMP_DURATION = 0.05f; // 50ms ramp

  std::array<AudioChannel, MAX_CHANNELS> m_channels;
  AudioQueue m_commandQueue;
  float m_masterVolume = 1.0f;
  float m_gameSpeed = 1.0f;
  bool m_shutdownRequested = false;
  float m_shutdownRampRemaining = 0.0f;

  void processCommands();
  void mixAudio(float *buffer, size_t frames);
  [[nodiscard]] int findFreeChannel();
};

}; // namespace ste