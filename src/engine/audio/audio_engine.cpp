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
  if (!m_active || !m_currentFile)
    return;

  // Update fading
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
  const float effectivePitch =
      m_pitch * m_playbackSpeed; // Combine m_pitch with speed

  for (size_t frame = 0; frame < frames; ++frame) {
    // Calculate the source m_position with combined m_pitch and speed
    float sourcePos = static_cast<float>(m_position) * effectivePitch;

    // Get interpolated sample(s)
    float sample = 0.0f;
    if (numChannels == 1) {
      sample = interpolateSample(sourcePos);
    } else {
      // For stereo files, interpolate both m_channels
      float leftSample = interpolateSample(sourcePos * 2.0f);
      float rightSample = interpolateSample(sourcePos * 2.0f + 1.0f);
      sample = (leftSample + rightSample) * 0.5f;
    }

    // Apply m_volume and spatialization
    float leftGain = m_volume;
    float rightGain = m_volume;
    applySpatialization(leftGain, rightGain);

    buffer[frame * 2] += sample * leftGain;
    buffer[frame * 2 + 1] += sample * rightGain;

    // Update m_position with speed-adjusted increment
    m_position += 1;
    if (m_position >= sourceSize / numChannels) {
      if (m_currentFile->isLooping()) {
        m_position = 0;
      } else {
        stop();
        break;
      }
    }
  }
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

float AudioChannel::interpolateSample(float m_position) const {
  if (!m_currentFile)
    return 0.0f;

  const float *data = m_currentFile->data();
  const size_t size = m_currentFile->size();

  // Linear interpolation between samples
  size_t pos1 = static_cast<size_t>(std::floor(m_position));
  size_t pos2 = pos1 + 1;
  float frac = m_position - static_cast<float>(pos1);

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