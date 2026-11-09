#pragma once

// Diagnostic-only cover logging for USB-locked devices: every entry goes to the
// serial log and is appended to /.crosspoint/cover-debug.log on the SD card.
// Remove once the missing-cover issue is understood.

#include <Arduino.h>
#include <HalStorage.h>
#include <Logging.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace CoverDiag {

constexpr char LOG_PATH[] = "/.crosspoint/cover-debug.log";
constexpr size_t MAX_LOG_BYTES = 64 * 1024;

inline void log(const char* format, ...) {
  char buf[256];
  const int prefixLen = snprintf(buf, sizeof(buf), "[%lu] ", static_cast<unsigned long>(millis()));
  if (prefixLen < 0) return;

  va_list args;
  va_start(args, format);
  vsnprintf(buf + prefixLen, sizeof(buf) - prefixLen, format, args);
  va_end(args);

  LOG_INF("CVR", "%s", buf + prefixLen);

  HalFile file = Storage.open(LOG_PATH, O_WRITE | O_CREAT | O_APPEND);
  if (!file) return;
  if (file.size() < MAX_LOG_BYTES) {
    file.write(reinterpret_cast<const uint8_t*>(buf), strlen(buf));
    file.write(static_cast<uint8_t>('\n'));
    file.flush();
  }
  file.close();
}

}  // namespace CoverDiag
