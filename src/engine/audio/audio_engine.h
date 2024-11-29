#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "audio_file.h"
#include "audio_queue.h"

namespace ste {

class AudioChannel {
public:
  AudioChannel() = default;
  ~AudioChannel() = default;

  void update(float deltaTime);
  void mix(float *buffer, size_t frames);
  void play(std::shared_ptr<AudioFile> file, float vol = 1.0f);
  void stop();
  void setVolume(float vol);
  void setPitch(float newPitch);
  void setPosition(float x, float y);
  void fadeVolume(float target, float duration);
  [[nodiscard]] bool isActive() const {
    return active && currentFile != nullptr;
  }

private:
  std::shared_ptr<AudioFile> currentFile;
  size_t position = 0;
  float volume = 1.0f;
  float targetVolume = 1.0f;
  float fadeTimeRemaining = 0.0f;
  float fadeDuration = 0.0f;
  float pitch = 1.0f;
  float positionX = 0.0f;
  float positionY = 0.0f;
  bool active = false;

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
  void playSound(const std::string &path, float volume = 1.0f);
  void playMusic(const std::string &path, bool loop = true);
  void stopChannel(int channelId);
  void stopAll();
  void setChannelVolume(int channelId, float volume);
  void setChannelPitch(int channelId, float pitch);
  void setChannelPosition(int channelId, float x, float y);
  void fadeChannel(int channelId, float targetVolume, float duration);
  void setMasterVolume(float volume);
  void beginShutdown() {
    shutdownRequested = true;
    shutdownRampRemaining = SHUTDOWN_RAMP_DURATION;
  }

  // Audio thread callback
  void audioCallback(float *buffer, size_t frames);

private:
  static constexpr size_t MAX_CHANNELS = 16;
  std::array<AudioChannel, MAX_CHANNELS> channels;
  AudioQueue commandQueue;
  float masterVolume = 1.0f;

  void processCommands();
  void mixAudio(float *buffer, size_t frames);
  [[nodiscard]] int findFreeChannel() const;

  static constexpr float SHUTDOWN_RAMP_DURATION = 0.05f; // 50ms ramp
  bool shutdownRequested = false;
  float shutdownRampRemaining = 0.0f;
};

}; // namespace ste