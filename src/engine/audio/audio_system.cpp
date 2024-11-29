#include "audio_system.h"

namespace ste {

std::size_t AudioSampleFormat::sizeOf() const {
  switch (m_format) {
  case UInt8:
    return sizeof(uint8_t);
  case Int16:
    return sizeof(int16_t);
  case Int32:
    return sizeof(int32_t);
  case Float32:
    return sizeof(float);
  case Float64:
    return sizeof(double);
  default:
    std::cerr << "Unknown audio sample format" << std::endl;
    return 0;
  }
}

std::size_t AudioSampleFormat::sizeOf(Type format) {
  switch (format) {
  case UInt8:
    return sizeof(uint8_t);
  case Int16:
    return sizeof(int16_t);
  case Int32:
    return sizeof(int32_t);
  case Float32:
    return sizeof(float);
  case Float64:
    return sizeof(double);
  default:
    std::cerr << "Unknown audio sample format" << std::endl;
    return 0;
  }
}

AudioSystem::~AudioSystem() {
  if (m_deviceId != 0) {
    SDL_PauseAudioDevice(m_deviceId, 1);
    SDL_CloseAudioDevice(m_deviceId);
    m_deviceId = 0;
  }
}

AudioSystem::AudioSystem(AudioSystem &&other) noexcept
    : m_deviceId(other.m_deviceId), m_config(other.m_config),
      m_paused(other.m_paused), m_callbackInstance(other.m_callbackInstance),
      m_audioCallback(std::move(other.m_audioCallback)),
      m_sampleFormat(other.m_sampleFormat) {
  if (m_deviceId != 0) {
    SDL_LockAudioDevice(m_deviceId);
    SDL_AudioSpec spec;
    SDL_GetAudioDeviceSpec(m_deviceId, 0, &spec);
    spec.userdata = this;
    SDL_UnlockAudioDevice(m_deviceId);
  }

  other.m_deviceId = 0;
  other.m_callbackInstance = nullptr;
}

AudioSystem &AudioSystem::operator=(AudioSystem &&other) noexcept {
  if (this != &other) {
    if (m_deviceId != 0) {
      SDL_PauseAudioDevice(m_deviceId, 1);
      SDL_CloseAudioDevice(m_deviceId);
    }

    m_deviceId = other.m_deviceId;
    m_config = other.m_config;
    m_paused = other.m_paused;
    m_callbackInstance = other.m_callbackInstance;
    m_audioCallback = std::move(other.m_audioCallback);
    m_sampleFormat = other.m_sampleFormat;

    if (m_deviceId != 0) {
      SDL_LockAudioDevice(m_deviceId);
      SDL_AudioSpec spec;
      SDL_GetAudioDeviceSpec(m_deviceId, 0, &spec);
      spec.userdata = this;
      SDL_UnlockAudioDevice(m_deviceId);
    }

    other.m_deviceId = 0;
    other.m_callbackInstance = nullptr;
  }
  return *this;
}

void AudioSystem::processAudio(Uint8 *stream, int len) {
  if (m_audioCallback && !m_paused) {
    // Calculate frames based on output format (assuming Float32)
    size_t numFrames = len / (sizeof(float) * m_config.numOutputChannels);

    // Process the audio
    m_audioCallback(m_config, stream, numFrames);
  } else {
    SDL_memset(stream, 0, len);
  }
}

void AudioSystem::pause() {
  if (!m_paused) {
    m_paused = true;
    SDL_PauseAudioDevice(m_deviceId, 1);
  }
}

void AudioSystem::resume() {
  if (m_paused) {
    m_paused = false;
    SDL_PauseAudioDevice(m_deviceId, 0);
  }
}
} // namespace ste