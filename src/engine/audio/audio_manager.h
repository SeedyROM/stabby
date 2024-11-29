#pragma once
#include "audio_engine.h"
#include "audio_system.h"

namespace ste {

// Adapts AudioEngine to work with AudioSystem
class AudioEngineAdapter {
public:
  static std::unique_ptr<AudioSystem> create(AudioEngineAdapter *adapter) {
    AudioSystem::CreateInfo createInfo;
    createInfo.config.sampleRate = 44100;
    createInfo.config.numOutputChannels = 2;
    createInfo.config.bufferSize = 1024;
    createInfo.config.sampleFormat = AudioSampleFormat::Float32;

    auto system = AudioSystem::create(
        adapter, &AudioEngineAdapter::processAudio, createInfo);

    if (system) {
      system->resume();
      return system;
    }
    return nullptr;
  }

  AudioEngine &getEngine() { return engine; }

private:
  AudioEngine engine;

  void processAudio(const AudioConfig &config, AudioBuffer<float> buffer) {
    // Convert AudioBuffer to raw float buffer for AudioEngine
    engine.audioCallback(buffer.data, buffer.numFrames);
  }
};

// Helper class to manage both systems
class AudioManager {
public:
  AudioManager() {
    adapter = std::make_unique<AudioEngineAdapter>();
    m_system = AudioEngineAdapter::create(adapter.get());
    if (!system) {
      throw std::runtime_error("Failed to initialize audio system");
    }
  }

  AudioEngine &getEngine() { return adapter->getEngine(); }

  void pause() {
    if (m_system)
      m_system->pause();
  }
  void resume() {
    if (m_system)
      m_system->resume();
  }
  bool isPaused() const { return m_system ? m_system->isPaused() : true; }

private:
  std::unique_ptr<AudioEngineAdapter>
      adapter; // Order matters - adapter must outlive system
  std::unique_ptr<AudioSystem> m_system;
};

} // namespace ste