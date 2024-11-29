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
  [[nodiscard]] const float *data() const { return samples.data(); }
  [[nodiscard]] size_t size() const { return samples.size(); }
  [[nodiscard]] uint32_t getSampleRate() const { return sampleRate; }
  [[nodiscard]] uint32_t getChannels() const { return channels; }
  [[nodiscard]] bool isLooping() const { return looping; }
  [[nodiscard]] const std::string &getFilename() const { return filename; }

  // Setters
  void setLooping(bool loop) { looping = loop; }

private:
  std::vector<float> samples;
  std::string filename;
  uint32_t sampleRate = 44100;
  uint32_t channels = 0;
  bool looping = false;

  // File format handlers
  void loadWAV(const std::string &path);
  void loadOGG(const std::string &path);

  // Utility functions
  static std::string getFileExtension(const std::string &path);
  void convertToFloat(const std::vector<int16_t> &pcmData);
};

}; // namespace ste