#pragma once

#include <cinttypes>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include <SDL2/SDL.h>

namespace ste {

class AudioSampleFormat {
public:
  enum Type { UInt8, Int16, Int32, Float32, Float64 };

private:
  Type m_format;

public:
  AudioSampleFormat(Type type = UInt8) : m_format(type) {}
  Type getFormat() const { return m_format; }
  std::size_t sizeOf() const;
  static std::size_t sizeOf(Type format);
  bool operator==(const AudioSampleFormat &other) const {
    return m_format == other.m_format;
  }
  bool operator!=(const AudioSampleFormat &other) const {
    return m_format != other.m_format;
  }
};

struct AudioRequestedConfig {
  int sampleRate;
  int numInputChannels;
  int numOutputChannels;
  int bufferSize;
  AudioSampleFormat sampleFormat;
};

struct AudioConfig {
  int sampleRate;
  int numInputChannels;
  int numOutputChannels;
  int bufferSize;
  AudioSampleFormat sampleFormat;
};

template <typename T> struct AudioBuffer {
  T *data;
  size_t numFrames;
  size_t numChannels;

  T &operator()(size_t frame, size_t channel) {
    return data[frame * numChannels + channel];
  }

  const T &operator()(size_t frame, size_t channel) const {
    return data[frame * numChannels + channel];
  }
};

class AudioSystem {
public:
  struct CreateInfo {
    std::string errorMsg;
    bool success;
    AudioRequestedConfig config;

    CreateInfo()
        : success(true), config{.sampleRate = 44100,
                                .numInputChannels = 0,
                                .numOutputChannels = 2,
                                .bufferSize = 1024,
                                .sampleFormat = AudioSampleFormat::Float32} {}
  };

  template <typename T>
  static std::unique_ptr<AudioSystem>
  create(T *instance,
         void (T::*callback)(const AudioConfig &, AudioBuffer<float>),
         CreateInfo createInfo = CreateInfo()) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
      createInfo.success = false;
      createInfo.errorMsg = SDL_GetError();
      return nullptr;
    }

    auto system = std::unique_ptr<AudioSystem>(new AudioSystem());
    system->setCallback(instance, callback);

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = createInfo.config.sampleRate;
    want.channels = createInfo.config.numOutputChannels;
    want.samples = createInfo.config.bufferSize;
    want.format = AUDIO_F32;
    want.callback = AudioSystem::sdlAudioCallback;
    want.userdata = system.get();

    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(
        nullptr, 0, &want, &have,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE |
            SDL_AUDIO_ALLOW_SAMPLES_CHANGE);

    if (deviceId == 0) {
      createInfo.success = false;
      createInfo.errorMsg = SDL_GetError();
      return nullptr;
    }

    AudioConfig config{.sampleRate = have.freq,
                       .numInputChannels = 0,
                       .numOutputChannels = have.channels,
                       .bufferSize = have.samples,
                       .sampleFormat = AudioSampleFormat::Float32};

    system->initWithDevice(deviceId, config);
    return system;
  }

  ~AudioSystem();
  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;
  AudioSystem(AudioSystem &&other) noexcept;
  AudioSystem &operator=(AudioSystem &&other) noexcept;

  const AudioConfig &getConfig() const { return m_config; }
  void pause();
  void resume();
  bool isPaused() const { return m_paused; }

private:
  AudioSystem() = default;

  void initWithDevice(SDL_AudioDeviceID deviceId, const AudioConfig &config) {
    m_deviceId = deviceId;
    m_config = config;
  }

  template <typename T>
  void setCallback(T *instance, void (T::*callback)(const AudioConfig &,
                                                    AudioBuffer<float>)) {
    m_callbackInstance = instance;
    m_audioCallback = [instance, callback](const AudioConfig &config,
                                           void *buffer, size_t numFrames) {
      AudioBuffer<float> audioBuffer{
          static_cast<float *>(buffer), numFrames,
          static_cast<size_t>(config.numOutputChannels)};
      (instance->*callback)(config, audioBuffer);
    };
    m_sampleFormat = AudioSampleFormat::Float32;
  }

  static void sdlAudioCallback(void *userdata, Uint8 *stream, int len) {
    auto *audio = static_cast<AudioSystem *>(userdata);
    if (audio) {
      audio->processAudio(stream, len);
    }
  }

  void processAudio(Uint8 *stream, int len);

  SDL_AudioDeviceID m_deviceId{0};
  AudioConfig m_config;
  bool m_paused{true};

  void *m_callbackInstance{nullptr};
  std::function<void(const AudioConfig &, void *, size_t)> m_audioCallback;
  AudioSampleFormat m_sampleFormat{AudioSampleFormat::Float32};
};

} // namespace ste