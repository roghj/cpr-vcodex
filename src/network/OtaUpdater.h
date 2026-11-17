#pragma once

#include <cstdint>
#include <string>

class OtaUpdater {
  bool updateAvailable = false;
  std::string latestVersion;
  std::string otaUrl;
  std::string expectedSha256;
  size_t otaSize = 0;
  size_t processedSize = 0;
  size_t totalSize = 0;

 public:
  using ProgressCallback = void (*)(void* ctx);

  enum OtaUpdaterError {
    OK = 0,
    NO_UPDATE,
    HTTP_ERROR,
    JSON_PARSE_ERROR,
    UPDATE_OLDER_ERROR,
    INTERNAL_UPDATE_ERROR,
    OOM_ERROR,
    WRONG_DEVICE_ERROR,
  };

  // Roghj: timings of the last installUpdate(), so a slow online update can be
  // diagnosed without a serial cable (OtaUpdateActivity writes them to the SD card).
  struct InstallStats {
    uint32_t beginMs = 0;     // esp_ota_begin (erases the target slot up front)
    uint32_t downloadMs = 0;  // HTTPS transfer, including the writes below
    uint32_t writeMs = 0;     // time inside esp_ota_write (flash)
    uint32_t hashMs = 0;      // SHA-256 + board-tag scan
    uint32_t finishMs = 0;    // esp_ota_end (image verification) + boot partition
    uint32_t totalMs = 0;
    uint32_t chunks = 0;  // body callbacks (chunk count)
    size_t bytes = 0;
    uint64_t writeUs = 0;  // accumulators behind writeMs / hashMs
    uint64_t hashUs = 0;
  };
  const InstallStats& getInstallStats() const { return installStats; }

 private:
  InstallStats installStats;

 public:
  size_t getOtaSize() const { return otaSize; }

  size_t getProcessedSize() const { return processedSize; }

  size_t getTotalSize() const { return totalSize; }

  void setProgress(size_t processed, size_t total) {
    processedSize = processed;
    totalSize = total;
  }

  OtaUpdater() = default;
  bool isUpdateNewer() const;
  const std::string& getLatestVersion() const;
  OtaUpdaterError checkForUpdate();
  OtaUpdaterError installUpdate(ProgressCallback onProgress = nullptr, void* ctx = nullptr);
};
