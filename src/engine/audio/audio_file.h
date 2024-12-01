// audio_file.h
#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ste {

class AudioFile {
public:
  struct CreateInfo {
    std::string errorMsg;
    bool success = true;
  };

  static std::optional<AudioFile> createFromFile(const std::string &path,
                                                 CreateInfo &createInfo);

  ~AudioFile();
  AudioFile(AudioFile &&other) noexcept;
  AudioFile &operator=(AudioFile &&other) noexcept;

  // Delete copy operations
  AudioFile(const AudioFile &) = delete;
  AudioFile &operator=(const AudioFile &) = delete;

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
  explicit AudioFile(const std::string &filename, std::vector<float> &&samples,
                     uint32_t sampleRate, uint32_t channels);

  static bool loadWAV(const std::string &path, std::vector<float> &samples,
                      uint32_t &sampleRate, uint32_t &channels,
                      CreateInfo &createInfo);

  static bool loadOGG(const std::string &path, std::vector<float> &samples,
                      uint32_t &sampleRate, uint32_t &channels,
                      CreateInfo &createInfo);

  static std::string getFileExtension(const std::string &path);
  static void convertToFloat(const std::vector<int16_t> &pcmData,
                             std::vector<float> &samples);

  std::vector<float> m_samples;
  std::string m_filename;
  uint32_t m_sampleRate = 44100;
  uint32_t m_channels = 0;
  bool m_looping = false;
};

} // namespace ste