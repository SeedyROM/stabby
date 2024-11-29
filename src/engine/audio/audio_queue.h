// audio_queue.h
#pragma once
#include "engine/async/spsc_queue.h"
#include <memory>
#include <string>

namespace ste {

class AudioFile; // Forward declaration

struct AudioCommand {
  enum class Type {
    Play,
    Stop,
    SetVolume,
    FadeIn,
    FadeOut,
    SetPitch,
    SetPosition,
    SetLoop
  };

  Type type;
  std::shared_ptr<AudioFile> file;
  float value1 = 0.0f; // volume, pitch, or x position
  float value2 = 0.0f; // fade time or y position
  int channelId = -1;
  bool flag = false; // For boolean parameters like loop

  // Default constructor for SPSCQueue
  AudioCommand() = default;

  // Copy constructor and assignment for queue operations
  AudioCommand(const AudioCommand &) = default;
  AudioCommand &operator=(const AudioCommand &) = default;
};

class AudioQueue {
  SPSCQueue<AudioCommand, 256> m_queue;

public:
  bool pushPlay(std::shared_ptr<AudioFile> file, float volume = 1.0f,
                int channelId = -1);
  bool pushStop(int channelId);
  bool pushVolume(int channelId, float volume);
  bool pushFade(int channelId, float targetVolume, float duration);
  bool pushPitch(int channelId, float pitch);
  bool pushPosition(int channelId, float x, float y);
  bool pushLoop(int channelId, bool shouldLoop);

  // New simplified command processing
  template <typename Handler>
  requires std::invocable<Handler, const AudioCommand &>
  void processCommands(Handler &&handler) {
    if (auto cmd = queue.try_pop()) {
      handler(*cmd);
      processCommands(handler); // Process all pending commands
    }
  }

  [[nodiscard]] bool empty() const;
  [[nodiscard]] bool full() const;
  void clear();
};

} // namespace ste