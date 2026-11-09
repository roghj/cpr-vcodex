#pragma once

// Diagnostic-only cover logging for USB-locked devices: every entry goes to the
// serial log and is appended to /.crosspoint/cover-debug.log on the SD card.
// ScopedCapture also records every other log line (ZIP, storage, inflate
// errors) emitted while a cover is being generated.
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
constexpr size_t CAPTURE_BYTES = 2048;

struct CaptureState {
  char buf[CAPTURE_BYTES];
  size_t len = 0;
  bool active = false;
  bool selfLogging = false;
};

inline CaptureState& capture() {
  static CaptureState state;
  return state;
}

inline void appendToSd(const char* text, const size_t len) {
  if (len == 0) return;
  HalFile file = Storage.open(LOG_PATH, O_WRITE | O_CREAT | O_APPEND);
  if (!file) return;
  if (file.size() < MAX_LOG_BYTES) {
    file.write(reinterpret_cast<const uint8_t*>(text), len);
    file.flush();
  }
  file.close();
}

// Runs inside logPrintf (possibly under a storage lock): buffer only, no I/O.
inline void tap(const char* line) {
  CaptureState& state = capture();
  if (!state.active || state.selfLogging || !line) return;
  static constexpr char PREFIX[] = "    sys: ";
  const size_t lineLen = strlen(line);
  const size_t needed = sizeof(PREFIX) - 1 + lineLen;
  if (state.len + needed >= CAPTURE_BYTES) return;
  memcpy(state.buf + state.len, PREFIX, sizeof(PREFIX) - 1);
  state.len += sizeof(PREFIX) - 1;
  memcpy(state.buf + state.len, line, lineLen);
  state.len += lineLen;
}

inline void flushCapture() {
  CaptureState& state = capture();
  const bool wasActive = state.active;
  state.active = false;
  appendToSd(state.buf, state.len);
  state.len = 0;
  state.active = wasActive;
}

inline void log(const char* format, ...) {
  char buf[256];
  const int prefixLen = snprintf(buf, sizeof(buf), "[%lu] ", static_cast<unsigned long>(millis()));
  if (prefixLen < 0) return;

  va_list args;
  va_start(args, format);
  vsnprintf(buf + prefixLen, sizeof(buf) - prefixLen, format, args);
  va_end(args);

  CaptureState& state = capture();
  state.selfLogging = true;
  LOG_INF("CVR", "%s", buf + prefixLen);
  state.selfLogging = false;

  // Keep captured system lines in order before this entry.
  flushCapture();
  const size_t len = strlen(buf);
  if (len + 1 < sizeof(buf)) {
    buf[len] = '\n';
    appendToSd(buf, len + 1);
  } else {
    appendToSd(buf, len);
  }
}

class ScopedCapture {
 public:
  ScopedCapture() {
    capture().len = 0;
    capture().active = true;
    setLogTap(&tap);
  }
  ~ScopedCapture() {
    setLogTap(nullptr);
    capture().active = false;
    flushCapture();
  }
  ScopedCapture(const ScopedCapture&) = delete;
  ScopedCapture& operator=(const ScopedCapture&) = delete;
};

}  // namespace CoverDiag
