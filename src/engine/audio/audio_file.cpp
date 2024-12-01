// audio_file.cpp
#include "audio_file.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <vorbis/vorbisfile.h>

namespace ste {

namespace {
// WAV file header structure
#pragma pack(push, 1)
struct WAVHeader {
  char riff[4];           // "RIFF"
  uint32_t fileSize;      // Total file size - 8
  char wave[4];           // "WAVE"
  char fmt[4];            // "fmt "
  uint32_t fmtSize;       // Format chunk size
  uint16_t audioFormat;   // Audio format (1 = PCM)
  uint16_t numChannels;   // Number of channels
  uint32_t sampleRate;    // Sample rate
  uint32_t byteRate;      // Bytes per second
  uint16_t blockAlign;    // Bytes per sample * channels
  uint16_t bitsPerSample; // Bits per sample
};
#pragma pack(pop)

// Helper function to check if a file exists
bool fileExists(const std::string &path) {
  std::ifstream file(path);
  return file.good();
}
} // namespace

std::optional<AudioFile> AudioFile::createFromFile(const std::string &path,
                                                   CreateInfo &createInfo) {
  if (!fileExists(path)) {
    createInfo.success = false;
    createInfo.errorMsg = "File not found: " + path;
    return std::nullopt;
  }

  std::vector<float> samples;
  uint32_t sampleRate;
  uint32_t channels;
  bool success = false;

  std::string ext = getFileExtension(path);
  if (ext == "wav") {
    success = loadWAV(path, samples, sampleRate, channels, createInfo);
  } else if (ext == "ogg") {
    success = loadOGG(path, samples, sampleRate, channels, createInfo);
  } else {
    createInfo.success = false;
    createInfo.errorMsg = "Unsupported file format: " + ext;
    return std::nullopt;
  }

  if (!success) {
    return std::nullopt;
  }

  return AudioFile(path, std::move(samples), sampleRate, channels);
}

AudioFile::AudioFile(const std::string &filename, std::vector<float> &&samples,
                     uint32_t sampleRate, uint32_t channels)
    : m_samples(std::move(samples)), m_filename(filename),
      m_sampleRate(sampleRate), m_channels(channels) {}

AudioFile::~AudioFile() = default;

AudioFile::AudioFile(AudioFile &&other) noexcept
    : m_samples(std::move(other.m_samples)),
      m_filename(std::move(other.m_filename)), m_sampleRate(other.m_sampleRate),
      m_channels(other.m_channels), m_looping(other.m_looping) {}

AudioFile &AudioFile::operator=(AudioFile &&other) noexcept {
  if (this != &other) {
    m_samples = std::move(other.m_samples);
    m_filename = std::move(other.m_filename);
    m_sampleRate = other.m_sampleRate;
    m_channels = other.m_channels;
    m_looping = other.m_looping;
  }
  return *this;
}

bool AudioFile::loadWAV(const std::string &path, std::vector<float> &samples,
                        uint32_t &sampleRate, uint32_t &channels,
                        CreateInfo &createInfo) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to open WAV file: " + path;
    return false;
  }

  // Read and validate header
  WAVHeader header;
  file.read(reinterpret_cast<char *>(&header), sizeof(WAVHeader));

  if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
      std::strncmp(header.wave, "WAVE", 4) != 0 ||
      std::strncmp(header.fmt, "fmt ", 4) != 0) {
    createInfo.success = false;
    createInfo.errorMsg = "Invalid WAV file format";
    return false;
  }

  if (header.audioFormat != 1) { // PCM = 1
    createInfo.success = false;
    createInfo.errorMsg = "Unsupported WAV format: non-PCM";
    return false;
  }

  if (header.bitsPerSample != 16) {
    createInfo.success = false;
    createInfo.errorMsg = "Unsupported WAV format: not 16-bit";
    return false;
  }

  // Find data chunk
  char chunkId[4];
  uint32_t chunkSize;
  while (file.read(chunkId, 4) &&
         file.read(reinterpret_cast<char *>(&chunkSize), 4)) {
    if (std::strncmp(chunkId, "data", 4) == 0) {
      break;
    }
    file.seekg(chunkSize, std::ios::cur);
  }

  // Read PCM data
  std::vector<int16_t> pcmData(chunkSize / sizeof(int16_t));
  file.read(reinterpret_cast<char *>(pcmData.data()), chunkSize);

  // Store audio properties
  sampleRate = header.sampleRate;
  channels = header.numChannels;

  // Convert to float samples
  convertToFloat(pcmData, samples);
  return true;
}

bool AudioFile::loadOGG(const std::string &path, std::vector<float> &samples,
                        uint32_t &sampleRate, uint32_t &channels,
                        CreateInfo &createInfo) {
  OggVorbis_File vf;
  if (ov_fopen(path.c_str(), &vf) != 0) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to open OGG file: " + path;
    return false;
  }

  // Get file info
  vorbis_info *vi = ov_info(&vf, -1);
  if (!vi) {
    createInfo.success = false;
    createInfo.errorMsg = "Failed to get OGG file info";
    ov_clear(&vf);
    return false;
  }

  sampleRate = static_cast<uint32_t>(vi->rate);
  channels = static_cast<uint32_t>(vi->channels);

  // Read all PCM data
  std::vector<int16_t> pcmData;
  int currentSection;
  char buffer[4096];
  long bytesRead;

  while ((bytesRead = ov_read(&vf, buffer, sizeof(buffer), 0, 2, 1,
                              &currentSection)) > 0) {
    size_t samplesRead = bytesRead / sizeof(int16_t);
    size_t currentSize = pcmData.size();
    pcmData.resize(currentSize + samplesRead);
    std::memcpy(pcmData.data() + currentSize, buffer, bytesRead);
  }

  if (bytesRead < 0) {
    createInfo.success = false;
    createInfo.errorMsg = "Error reading OGG file data";
    ov_clear(&vf);
    return false;
  }

  ov_clear(&vf);

  // Convert to float samples
  convertToFloat(pcmData, samples);
  return true;
}

void AudioFile::convertToFloat(const std::vector<int16_t> &pcmData,
                               std::vector<float> &samples) {
  samples.resize(pcmData.size());
  const float scale = 1.0f / 32768.0f; // Scale factor for 16-bit audio

  // Convert samples from int16 to float
  for (size_t i = 0; i < pcmData.size(); ++i) {
    samples[i] = static_cast<float>(pcmData[i]) * scale;
  }
}

std::string AudioFile::getFileExtension(const std::string &path) {
  size_t pos = path.find_last_of('.');
  if (pos != std::string::npos) {
    std::string ext = path.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
  }
  return "";
}

} // namespace ste