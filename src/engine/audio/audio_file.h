#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace ste {

class AudioFileException : public std::runtime_error {
public:
  explicit AudioFileException(const std::string &message)
      : std::runtime_error(message) {}
};

class AudioFile {
public:
  explicit AudioFile(const std::string &path);
  ~AudioFile();

  // Delete copy operations to prevent accidental copies
  AudioFile(const AudioFile &) = delete;
  AudioFile &operator=(const AudioFile &) = delete;

  // Allow move operations
  AudioFile(AudioFile &&) noexcept;
  AudioFile &operator=(AudioFile &&) noexcept;

  // Getters
  [[nodiscard]] const float *data() const { return m_samples.data(); }
  [[nodiscard]] size_t size() const { return m_samples.size(); }
  [[nodiscard]] uint32_t getSampleRate() const { return m_sampleRate; }
  [[nodiscard]] uint32_t getChannels() const { return m_channels; }
  [[nodiscard]] bool isLooping() const { return m_looping; }
  [[nodiscard]] const std::string &getFilename() const { return m_filename; }

  // Setters
  void setLooping(bool loop) { m_looping = loop; }

private:
  std::vector<float> m_samples;
  std::string m_filename;
  uint32_t m_sampleRate = 44100;
  uint32_t m_channels = 0;
  bool m_looping = false;

  // File format handlers
  void loadWAV(const std::string &path);
  void loadOGG(const std::string &path);

  // Utility functions
  static std::string getFileExtension(const std::string &path);
  void convertToFloat(const std::vector<int16_t> &pcmData);
};

}; // namespace ste