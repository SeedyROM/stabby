// audio_m_queue.cpp
#include "audio_queue.h"
#include "audio_file.h"

namespace ste {

bool AudioQueue::pushPlay(AudioFile *file, float volume = 1.0f,
                          int channelId = -1) {
  AudioCommand cmd;
  cmd.type = AudioCommand::Type::Play;
  cmd.file =
      file; // Store the raw pointer - the AssetHandle maintains ownership
  cmd.value1 = volume;
  cmd.channelId = channelId;
  return m_queue.try_push(cmd);
}

bool AudioQueue::pushStop(int channelId) {
  AudioCommand cmd;
  cmd.type = AudioCommand::Type::Stop;
  cmd.channelId = channelId;
  return m_queue.try_push(cmd);
}

bool AudioQueue::pushVolume(int channelId, float volume) {
  AudioCommand cmd;
  cmd.type = AudioCommand::Type::SetVolume;
  cmd.value1 = volume;
  cmd.channelId = channelId;
  return m_queue.try_push(cmd);
}

bool AudioQueue::pushFade(int channelId, float targetVolume, float duration) {
  AudioCommand cmd;
  cmd.type = targetVolume > 0.0f ? AudioCommand::Type::FadeIn
                                 : AudioCommand::Type::FadeOut;
  cmd.value1 = std::abs(targetVolume);
  cmd.value2 = duration;
  cmd.channelId = channelId;
  return m_queue.try_push(cmd);
}

bool AudioQueue::pushPitch(int channelId, float pitch) {
  AudioCommand cmd;
  cmd.type = AudioCommand::Type::SetPitch;
  cmd.value1 = pitch;
  cmd.channelId = channelId;
  return m_queue.try_push(cmd);
}

bool AudioQueue::pushPosition(int channelId, float x, float y) {
  AudioCommand cmd;
  cmd.type = AudioCommand::Type::SetPosition;
  cmd.value1 = x;
  cmd.value2 = y;
  cmd.channelId = channelId;
  return m_queue.try_push(cmd);
}

bool AudioQueue::pushLoop(int channelId, bool shouldLoop) {
  AudioCommand cmd;
  cmd.type = AudioCommand::Type::SetLoop;
  cmd.channelId = channelId;
  cmd.flag = shouldLoop;
  return m_queue.try_push(cmd);
}

bool AudioQueue::empty() const { return m_queue.empty(); }

bool AudioQueue::full() const { return m_queue.full(); }

void AudioQueue::clear() {
  while (m_queue.try_pop()) {
  }
}

} // namespace ste