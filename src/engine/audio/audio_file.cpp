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

AudioFile::AudioFile(const std::string &path) : filename(path) {
  if (!fileExists(path)) {
    throw AudioFileException("File not found: " + path);
  }

  std::string ext = getFileExtension(path);
  if (ext == "wav") {
    loadWAV(path);
  } else if (ext == "ogg") {
    loadOGG(path);
  } else {
    throw AudioFileException("Unsupported file format: " + ext);
  }
}

AudioFile::~AudioFile() = default;

AudioFile::AudioFile(AudioFile &&other) noexcept
    : samples(std::move(other.samples)), filename(std::move(other.filename)),
      sampleRate(other.sampleRate), channels(other.channels),
      looping(other.looping) {}

AudioFile &AudioFile::operator=(AudioFile &&other) noexcept {
  if (this != &other) {
    samples = std::move(other.samples);
    filename = std::move(other.filename);
    sampleRate = other.sampleRate;
    channels = other.channels;
    looping = other.looping;
  }
  return *this;
}

void AudioFile::loadWAV(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw AudioFileException("Failed to open WAV file: " + path);
  }

  // Read and validate header
  WAVHeader header;
  file.read(reinterpret_cast<char *>(&header), sizeof(WAVHeader));

  if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
      std::strncmp(header.wave, "WAVE", 4) != 0 ||
      std::strncmp(header.fmt, "fmt ", 4) != 0) {
    throw AudioFileException("Invalid WAV file format");
  }

  if (header.audioFormat != 1) { // PCM = 1
    throw AudioFileException("Unsupported WAV format: non-PCM");
  }

  if (header.bitsPerSample != 16) {
    throw AudioFileException("Unsupported WAV format: not 16-bit");
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
  convertToFloat(pcmData);
}

void AudioFile::loadOGG(const std::string &path) {
  OggVorbis_File vf;
  if (ov_fopen(path.c_str(), &vf) != 0) {
    throw AudioFileException("Failed to open OGG file: " + path);
  }

  // Get file info
  vorbis_info *vi = ov_info(&vf, -1);
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

  ov_clear(&vf);

  // Convert to float samples
  convertToFloat(pcmData);
}

void AudioFile::convertToFloat(const std::vector<int16_t> &pcmData) {
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

}; // namespace ste