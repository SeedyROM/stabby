#include "audio_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

#include <SDL2/SDL.h>

namespace ste {

constexpr float PI = 3.14159265358979323846f;
constexpr float DISTANCE_FALLOFF = 1.0f;
constexpr float MAX_DISTANCE = 10.0f;

void AudioChannel::setPlaybackSpeed(float speed) {
  m_targetSpeed = std::clamp(speed, 0.1f, 3.0f);
}

void AudioChannel::update(float deltaTime) {
  if (!m_active || !m_currentFile)
    return;

  m_currentSpeed = std::lerp(m_currentSpeed, m_targetSpeed, deltaTime * 8.0f);

  if (m_fadeTimeRemaining > 0.0f) {
    m_fadeTimeRemaining = std::max(0.0f, m_fadeTimeRemaining - deltaTime);
    float t = 1.0f - (m_fadeTimeRemaining / m_fadeDuration);
    m_volume = std::lerp(m_volume, m_targetVolume, t);

    if (m_fadeTimeRemaining == 0.0f) {
      if (m_volume <= 0.0f) {
        stop();
      }
    }
  }
}

void AudioChannel::mix(float *buffer, size_t frames) {
  if (!m_active || !m_currentFile || m_volume <= 0.0f)
    return;

  const float *sourceData = m_currentFile->data();
  const size_t sourceSize = m_currentFile->size();
  const size_t numChannels = m_currentFile->getChannels();
  const float effectivePitch = m_pitch * m_currentSpeed;

  float readPosition = static_cast<float>(m_position);

  for (size_t frame = 0; frame < frames; ++frame) {
    float sample = 0.0f;
    if (numChannels == 1) {
      sample = interpolateSample(readPosition);
    } else {
      float leftSample = interpolateSample(readPosition * 2.0f);
      float rightSample = interpolateSample(readPosition * 2.0f + 1.0f);
      sample = (leftSample + rightSample) * 0.5f;
    }

    float leftGain = m_volume;
    float rightGain = m_volume;
    applySpatialization(leftGain, rightGain);

    buffer[frame * 2] += sample * leftGain;
    buffer[frame * 2 + 1] += sample * rightGain;

    readPosition += effectivePitch;
    if (static_cast<size_t>(readPosition) >= sourceSize / numChannels) {
      if (m_currentFile->isLooping()) {
        readPosition = 0;
      } else {
        stop();
        break;
      }
    }
  }

  m_position = static_cast<size_t>(readPosition);
}

void AudioChannel::play(std::shared_ptr<AudioFile> file, float vol) {
  m_currentFile = std::move(file);
  m_position = 0;
  m_volume = vol;
  m_targetVolume = vol;
  m_fadeTimeRemaining = 0.0f;
  m_active = true;
}

void AudioChannel::stop() {
  m_active = false;
  m_currentFile.reset();
  m_position = 0;
}

void AudioChannel::setVolume(float vol) {
  m_volume = std::clamp(vol, 0.0f, 1.0f);
  m_targetVolume = m_volume;
  m_fadeTimeRemaining = 0.0f;
}

void AudioChannel::setPitch(float newPitch) {
  m_pitch = std::clamp(newPitch, 0.1f, 3.0f);
}

void AudioChannel::setPosition(float x, float y) {
  m_positionX = x;
  m_positionY = y;
}

void AudioChannel::fadeVolume(float target, float duration) {
  target = std::clamp(target, 0.0f, 1.0f);
  if (duration <= 0.0f) {
    m_volume = target;
    m_targetVolume = target;
    m_fadeTimeRemaining = 0.0f;
  } else {
    m_targetVolume = target;
    m_fadeDuration = duration;
    m_fadeTimeRemaining = duration;
  }
}

float AudioChannel::calculatePanLeft() const {
  if (m_positionX == 0.0f)
    return 1.0f;
  return std::clamp(1.0f - m_positionX * 0.5f, 0.0f, 1.0f);
}

float AudioChannel::calculatePanRight() const {
  if (m_positionX == 0.0f)
    return 1.0f;
  return std::clamp(1.0f + m_positionX * 0.5f, 0.0f, 1.0f);
}

void AudioChannel::applySpatialization(float &left, float &right) const {
  constexpr float DISTANCE_FALLOFF = 1.0f;

  // Calculate distance-based attenuation
  float distance =
      std::sqrt(m_positionX * m_positionX + m_positionY * m_positionY);
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
  if (!m_currentFile) {
    return 0.0f;
  }

  const float *data = m_currentFile->data();
  const size_t size = m_currentFile->size();

  // Get integer position and fractional part
  size_t pos1 = static_cast<size_t>(std::floor(position));
  float frac = position - static_cast<float>(pos1);

  // Get the sample points
  size_t pos0 = (pos1 == 0) ? pos1 : pos1 - 1;        // Previous sample
  size_t pos2 = (pos1 + 1 >= size) ? pos1 : pos1 + 1; // Next sample
  size_t pos3 = (pos1 + 2 >= size) ? pos2 : pos1 + 2; // Next next sample

  float p0 = data[pos0];
  float p1 = data[pos1];
  float p2 = data[pos2];
  float p3 = data[pos3];

  // Cubic Hermite coefficients
  float t = frac;
  float t2 = t * t;
  float t3 = t2 * t;

  // Hermite basis functions
  float h0 = -0.5f * t3 + t2 - 0.5f * t;
  float h1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
  float h2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
  float h3 = 0.5f * t3 - 0.5f * t2;

  // Interpolate
  return p0 * h0 + p1 * h1 + p2 * h2 + p3 * h3;
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
  // Initialize m_channels array
  for (auto &channel : m_channels) {
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

void AudioEngine::playSound(const std::string &path, float m_volume) {
  try {
    auto file = std::make_shared<AudioFile>(path);
    int channel = findFreeChannel();
    if (channel != -1) {
      m_commandQueue.pushPlay(file, m_volume, channel);
    }
  } catch (const AudioFileException &e) {
    std::cerr << "Failed to load sound file: " << e.what() << std::endl;
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
    m_commandQueue.pushPlay(file, 1.0f, 0); // Channel 0 reserved for music
  } catch (const AudioFileException &e) {
    std::cerr << "Failed to load sound file: " << e.what() << std::endl;

    // Handle or propagate error
  }
}

void AudioEngine::stopChannel(int channelId) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    m_commandQueue.pushStop(channelId);
  }
}

void AudioEngine::stopAll() {
  for (int i = 0; i < static_cast<int>(MAX_CHANNELS); ++i) {
    stopChannel(i);
  }
  m_commandQueue.clear();
}

void AudioEngine::setChannelVolume(int channelId, float m_volume) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    m_commandQueue.pushVolume(channelId, std::clamp(m_volume, 0.0f, 1.0f));
  }
}

void AudioEngine::setChannelPitch(int channelId, float m_pitch) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    m_commandQueue.pushPitch(channelId, std::clamp(m_pitch, 0.1f, 3.0f));
  }
}

void AudioEngine::setChannelPosition(int channelId, float x, float y) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    m_commandQueue.pushPosition(channelId, x, y);
  }
}

void AudioEngine::fadeChannel(int channelId, float m_targetVolume,
                              float duration) {
  if (channelId >= 0 && channelId < static_cast<int>(MAX_CHANNELS)) {
    m_commandQueue.pushFade(channelId, m_targetVolume,
                            std::max(0.0f, duration));
  }
}

void AudioEngine::setMasterVolume(float m_volume) {
  m_masterVolume = std::clamp(m_volume, 0.0f, 1.0f);
}

void AudioEngine::processCommands() {
  m_commandQueue.processCommands([this](const AudioCommand &cmd) {
    if (cmd.channelId < 0 || cmd.channelId >= static_cast<int>(MAX_CHANNELS)) {
      return;
    }

    auto &channel = m_channels[cmd.channelId];
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
  processCommands();
  std::memset(buffer, 0, frames * DEFAULT_CHANNELS * sizeof(float));

  size_t m_activeChannels = 0;
  const float framesPerSecond = static_cast<float>(DEFAULT_SAMPLE_RATE);
  float deltaTime = frames / framesPerSecond;

  // Calculate shutdown ramp if needed
  float shutdownRamp = 1.0f;
  if (m_shutdownRequested) {
    shutdownRamp =
        std::max(0.0f, m_shutdownRampRemaining / SHUTDOWN_RAMP_DURATION);
    m_shutdownRampRemaining -= deltaTime;
  }

  // Mix all m_active m_channels with speed control
  for (auto &channel : m_channels) {
    if (channel.isActive()) {
      // Update channel with speed-adjusted time
      channel.update(deltaTime * m_gameSpeed);
      channel.setPlaybackSpeed(m_gameSpeed);
      channel.mix(buffer, frames);
      m_activeChannels++;
    }
  }

  // Normalize and apply master m_volume
  if (m_activeChannels > 0) {
    float peakAmplitude = 0.0f;
    for (size_t i = 0; i < frames * DEFAULT_CHANNELS; ++i) {
      peakAmplitude = std::max(peakAmplitude, std::abs(buffer[i]));
    }

    float normalizationFactor =
        (peakAmplitude > 1.0f) ? 1.0f / peakAmplitude : 1.0f;
    float finalGain = normalizationFactor * m_masterVolume * shutdownRamp;

    for (size_t i = 0; i < frames * DEFAULT_CHANNELS; ++i) {
      buffer[i] = std::clamp(buffer[i] * finalGain, -1.0f, 1.0f);
    }
  }
}

int AudioEngine::findFreeChannel() const {
  // Skip channel 0 (reserved for music)
  for (size_t i = 1; i < MAX_CHANNELS; ++i) {
    if (!m_channels[i].isActive()) {
      return static_cast<int>(i);
    }
  }
  return -1; // No free m_channels available
}

}; // namespace ste