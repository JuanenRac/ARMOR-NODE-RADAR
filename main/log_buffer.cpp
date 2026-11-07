// ARMOR-NODE-RADAR - the last few kilobytes of the node's log, so the panel can show what the console would.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "log_buffer.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
extern "C" {
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
}
#include "core/log_line.hpp"

namespace armor::logbuf {
namespace {
constexpr std::size_t kSize = 8192;
char g_ring[kSize];
std::uint32_t g_total = 0;   // bytes ever written; the byte at position p is g_ring[p % kSize]
portMUX_TYPE g_spin = portMUX_INITIALIZER_UNLOCKED;
vprintf_like_t g_previous = nullptr;

void append(const char* text, std::size_t length) {
  portENTER_CRITICAL(&g_spin);
  for (std::size_t i = 0; i < length; ++i) g_ring[(g_total + i) % kSize] = text[i];
  g_total += static_cast<std::uint32_t>(length);
  portEXIT_CRITICAL(&g_spin);
}

// One buffer for every task, not 224 bytes on the stack of each one that logs. The lock is only tried: a task never waits to write a log line, and the one that finds the buffer taken formats into a small
// one of its own (so its line is cut shorter, never lost). The console gets the line from the same arguments as before, so what it shows does not depend on this buffer.
char g_line[kSharedLine];
std::mutex g_line_lock;

int hook(const char* format, va_list args) {
  va_list copy;
  va_copy(copy, args);
  if (g_line_lock.try_lock()) {
    const int written = std::vsnprintf(g_line, sizeof g_line, format, copy);
    const std::size_t kept = kept_length(written, sizeof g_line);
    if (kept > 0) append(g_line, kept);
    g_line_lock.unlock();
  } else {
    char spare[kSpareLine];
    const int written = std::vsnprintf(spare, sizeof spare, format, copy);
    const std::size_t kept = kept_length(written, sizeof spare);
    if (kept > 0) append(spare, kept);
  }
  va_end(copy);
  return g_previous != nullptr ? g_previous(format, args) : std::vprintf(format, args);
}
}  // namespace

void start() { g_previous = esp_log_set_vprintf(&hook); }

std::string read_from(std::uint32_t from, std::uint32_t& next, std::size_t max_bytes) {
  portENTER_CRITICAL(&g_spin);
  const std::uint32_t total = g_total;   // only the counter is read with interrupts off; the text is copied after
  portEXIT_CRITICAL(&g_spin);
  std::uint32_t begin = from;
  if (begin > total) begin = total;
  if (total - begin > kSize) begin = total - static_cast<std::uint32_t>(kSize);   // the ring has moved past what was asked
  if (total - begin > max_bytes) begin = total - static_cast<std::uint32_t>(max_bytes);
  std::string out;
  out.reserve(total - begin);
  for (std::uint32_t p = begin; p < total; ++p) out += g_ring[p % kSize];   // a line being written right now may come out garbled: harmless
  next = total;
  return out;
}

}  // namespace armor::logbuf
