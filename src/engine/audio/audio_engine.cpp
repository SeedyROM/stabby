#include "audio_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <SDL2/SDL.h>

namespace ste {

constexpr float PI = 3.14159265358979323846f;
constexpr float DISTANCE_FALLOFF = 1.0f;
constexpr float MAX_DISTANCE = 10.0f;

void AudioChannel::update(float deltaTime) {
  if (!active || !currentFile)
    return;

  // Update fading
  if (fadeTimeRemaining > 0.0f) {
    fadeTimeRemaining = std::max(0.0f, fadeTimeRemaining - deltaTime);
    float t = 1.0f - (fadeTimeRemaining / fadeDuration);
    volume = std::lerp(volume, targetVolume, t);

    if (fadeTimeRemaining == 0.0f) {
      if (volume <= 0.0f) {
        stop();
      }
    }
  }
}

void AudioChannel::mix(float *buffer, size_t frames) {
  if (!active || !currentFile || volume <= 0.0f)
    return;

  const float *sourceData = currentFile->data();
  const size_t sourceSize = currentFile->size();
  const size_t numChannels = currentFile->getChannels();

  for (size_t frame = 0; frame < frames; ++frame) {
    // Calculate the source position with pitch
    float sourcePos = static_cast<float>(position) * pitch;

    // Get interpolated sample(s)
    float sample = 0.0f;
    if (numChannels == 1) {
      sample = interpolateSample(sourcePos);
    } else {
      // For stereo files, interpolate both channels
      float leftSample = interpolateSample(sourcePos * 2.0f);
      float rightSample = interpolateSample(sourcePos * 2.0f + 1.0f);
      sample = (leftSample + rightSample) * 0.5f;
    }

    // Apply volume and spatialization
    float leftGain = volume;
    float rightGain = volume;
    applySpatialization(leftGain, rightGain);

    buffer[frame * 2] += sample * leftGain;
    buffer[frame * 2 + 1] += sample * rightGain;

    // Update position
    position += 1;
    if (position >= sourceSize / numChannels) {
      if (currentFile->isLooping()) {
        position = 0;
      } else {
        stop();
        break;
      }
    }
  }
}

void AudioChannel::play(std::shared_ptr<AudioFile> file, float vol) {
  currentFile = std::move(file);
  position = 0;
  volume = vol;
  targetVolume = vol;
  fadeTimeRemaining = 0.0f;
  active = true;
}

void AudioChannel::stop() {
  active = false;
  currentFile.reset();
  position = 0;
}

void AudioChannel::setVolume(float vol) {
  volume = std::clamp(vol, 0.0f, 1.0f);
  targetVolume = volume;
  fadeTimeRemaining = 0.0f;
}

void AudioChannel::setPitch(float newPitch) {
  pitch = std::clamp(newPitch, 0.1f, 3.0f);
}

void AudioChannel::setPosition(float x, float y) {
  positionX = x;
  positionY = y;
}

void AudioChannel::fadeVolume(float target, float duration) {
  target = std::clamp(target, 0.0f, 1.0f);
  if (duration <= 0.0f) {
    volume = target;
    targetVolume = target;
    fadeTimeRemaining = 0.0f;
  } else {
    targetVolume = target;
    fadeDuration = duration;
    fadeTimeRemaining = duration;
  }
}

float AudioChannel::calculatePanLeft() const {
  if (positionX == 0.0f)
    return 1.0f;
  return std::clamp(1.0f - positionX * 0.5f, 0.0f, 1.0f);
}

float AudioChannel::calculatePanRight() const {
  if (positionX == 0.0f)
    return 1.0f;
  return std::clamp(1.0f + positionX * 0.5f, 0.0f, 1.0f);
}

void AudioChannel::applySpatialization(float &left, float &right) const {
  // Calculate distance-based attenuation
  float distance = std::sqrt(positionX * positionX + positionY * positionY);
  float attenuation = 1.0f;
  if (distance > 0.0f) {
    attenuation = std::min(1.0f, 1.0f / (1.0f + DISTANCE_FALLOFF * distance));
    attenuation = std::pow(attenuation, 2.0f); // Quadratic falloff
  }

  // Apply panning
  left *= calculatePanLeft() * attenuation;
  right *= calculatePanRight() * attenuation;
}

float AudioChannel::interpolateSample(float position) const {
  if (!currentFile)
    return 0.0f;

  const float *data = currentFile->data();
  const size_t size = currentFile->size();

  // Linear interpolation between samples
  size_t pos1 = static_cast<size_t>(std::floor(position));
  size_t pos2 = pos1 + 1;
  float frac = position - static_cast<float>(pos1);

  // Handle edge cases
  if (pos1 >= size)
    return 0.0f;
  if (pos2 >= size)
    pos2 = pos1;

  return std::lerp(data[pos1], data[pos2], frac);
}

namespace {
// Audio callback function for the backend
void audioCallback(void *userData, float *buffer, size_t frames) {
  auto *engine = static_cast<AudioEngine *>(userData);
  engine->audioCallback(buffer, frames);
}

// Default audio settings
constexpr uint32_t DEFAULT_SAMPLE_RATE = 44100;
constexpr uint32_t DEFAULT_CHANNELS = 2;
constexpr uint32_t DEFAULT_BUFFER_SIZE = 1024;
} // namespace

AudioEngine::AudioEngine() {
  // Initialize channels array
  for (auto &channel : channels) {
    channel.setVolume(1.0f);
    channel.setPitch(1.0f);
    channel.setPosition(0.0f, 0.0f);
  }
}

AudioEngine::~AudioEngine() {
  beginShutdown();
  // Wait for ramp duration before actual cleanup
  SDL_Delay(static_cast<Uint32>(SHUTDOWN_RAMP_DURATION * 1000));
  stopAll();
}

void AudioEngine::playSound(const std::string &path, float volume) {
  try {
    auto file = std::make_shared<AudioFile>(path);
    int channel = findFreeChannel();
    if (channel != -1) {
      commandQueue.pushPlay(file, volume, channel);
    }
  } catch (const AudioFileException &e) {
    // Handle or propagate error
    // Could log error or throw depending on your error handling strategy
  }
}

void AudioEngine::playMusic(const std::string &path, bool loop) {
  try {
    auto file = std::make_shared<AudioFile>(path);
    file->setLooping(loop);

    // Stop any currently playing music
    stopChannel(0);

    // Queue the new music
    commandQueue.pushPlay(file, 1.0f, 0); // Channel 0 reserved for music
  } catch (const AudioFileException &e) {
    // Handle or propagate error
  }
}

void AudioEngine::stopChannel(int channelId) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    commandQueue.pushStop(channelId);
  }
}

void AudioEngine::stopAll() {
  for (int i = 0; i < static_cast<int>(MAX_CHANNELS); ++i) {
    stopChannel(i);
  }
  commandQueue.clear();
}

void AudioEngine::setChannelVolume(int channelId, float volume) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    commandQueue.pushVolume(channelId, std::clamp(volume, 0.0f, 1.0f));
  }
}

void AudioEngine::setChannelPitch(int channelId, float pitch) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    commandQueue.pushPitch(channelId, std::clamp(pitch, 0.1f, 3.0f));
  }
}

void AudioEngine::setChannelPosition(int channelId, float x, float y) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    commandQueue.pushPosition(channelId, x, y);
  }
}

void AudioEngine::fadeChannel(int channelId, float targetVolume,
                              float duration) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    commandQueue.pushFade(channelId, targetVolume, std::max(0.0f, duration));
  }
}

void AudioEngine::setMasterVolume(float volume) {
  masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioEngine::processCommands() {
  commandQueue.processCommands([this](const AudioCommand &cmd) {
    if (cmd.channelId < 0 || cmd.channelId >= static_cast<int>(MAX_CHANNELS)) {
      return;
    }

    auto &channel = channels[cmd.channelId];
    switch (cmd.type) {
    case AudioCommand::Type::Play:
      if (cmd.file) {
        channel.play(cmd.file, cmd.value1);
      }
      break;

    case AudioCommand::Type::Stop:
      channel.stop();
      break;

    case AudioCommand::Type::SetVolume:
      channel.setVolume(cmd.value1);
      break;

    case AudioCommand::Type::FadeIn:
    case AudioCommand::Type::FadeOut:
      channel.fadeVolume(cmd.value1, cmd.value2);
      break;

    case AudioCommand::Type::SetPitch:
      channel.setPitch(cmd.value1);
      break;

    case AudioCommand::Type::SetPosition:
      channel.setPosition(cmd.value1, cmd.value2);
      break;

    case AudioCommand::Type::SetLoop:
      if (cmd.file) {
        cmd.file->setLooping(cmd.flag);
      }
      break;
    }
  });
}

void AudioEngine::audioCallback(float *buffer, size_t frames) {
  // Process any pending commands
  processCommands();

  // Clear the output buffer
  std::memset(buffer, 0, frames * DEFAULT_CHANNELS * sizeof(float));

  size_t activeChannels = 0;

  // Mix all active channels directly into the output buffer
  for (auto &channel : channels) {
    if (channel.isActive()) {
      channel.update(static_cast<float>(frames) / DEFAULT_SAMPLE_RATE);
      channel.mix(buffer, frames);
      activeChannels++;
    }
  }

  // If we have active channels, normalize and apply master volume in a single
  // pass
  if (activeChannels > 0) {
    // First frame pass: find peak amplitude
    float peakAmplitude = 0.0f;
    for (size_t i = 0; i < frames * DEFAULT_CHANNELS; ++i) {
      peakAmplitude = std::max(peakAmplitude, std::abs(buffer[i]));
    }

    // Calculate normalization factor
    float normalizationFactor =
        (peakAmplitude > 1.0f) ? 1.0f / peakAmplitude : 1.0f;
    float finalGain = normalizationFactor * masterVolume;

    // Apply normalization, master volume, and safety limiting in one pass
    for (size_t i = 0; i < frames * DEFAULT_CHANNELS; ++i) {
      buffer[i] = std::clamp(buffer[i] * finalGain, -1.0f, 1.0f);
    }
  }
}

int AudioEngine::findFreeChannel() const {
  // Skip channel 0 (reserved for music)
  for (size_t i = 1; i < MAX_CHANNELS; ++i) {
    if (!channels[i].isActive()) {
      return static_cast<int>(i);
    }
  }
  return -1; // No free channels available
}

}; // namespace ste