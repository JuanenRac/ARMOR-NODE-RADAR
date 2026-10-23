// ARMOR-NODE-RADAR - the rules of the transparent serial bridge to one sensor. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The manufacturers' tools for a PC (the configuration tools of the sensors, and their flashing tools) speak to a sensor over a serial cable. The bridge lets an administrator use them through the
// node: the node's serial port to the sensor is handed to a session, the bytes the tool writes go to the sensor and the bytes the sensor sends go back, and the node understands none of it (it does not
// know how a sensor is flashed). The node's own reading of that sensor is paused while the session lasts.
//
// What this file decides, and the firmware obeys:
//   - only an administrator opens a session, for one sensor at a time (a second request is refused while a session is open), and it ends by itself after a few minutes;
//   - the tool presents the session's secret (a random token) with every exchange, compared in constant time, and the exchange also needs the administrator's login like any panel request;
//   - each exchange is small (writes up to 512 bytes, reads up to 1024) and the bytes per second are limited, so a runaway tool cannot hold the node or fill its memory;
//   - the speed is one of the sensors' speeds;
//   - everything that happens (opened, closed by the administrator, expired, refused) is a short record the node puts in its log and in its history.
// The sensor's power is always 5 V from the node: there is no switch for it, so a tool that needs the sensor to start in its bootloader, or to open its Bluetooth window, needs the sensor's supply
// cut by hand. The panel says so.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace armor::bridge {

constexpr std::size_t kMaxWrite = 512;
constexpr std::size_t kMaxRead = 1024;
constexpr std::uint32_t kDefaultMinutes = 10;
constexpr std::uint32_t kMaxMinutes = 30;
constexpr std::size_t kTokenBytes = 16;
constexpr std::uint32_t kBytesPerSecond = 40000;   // both ways together: a 256000 baud line carries 25600 bytes a second

constexpr std::array<std::uint32_t, 9> kBauds = {9600, 19200, 38400, 57600, 115200, 230400, 256000, 460800, 921600};
inline bool baud_allowed(std::uint32_t baud) { for (std::uint32_t allowed : kBauds) if (baud == allowed) return true; return false; }

enum class Refusal : std::uint8_t {
  kNone,
  kBusy,          // another session is open
  kBadSensor,
  kBadBaud,
  kBadMinutes,
  kNoSession,
  kBadToken,
  kExpired,
  kTooBig,
  kTooFast,       // more bytes than the line can carry
};

inline const char* refusal_text(Refusal r) {
  switch (r) {
    case Refusal::kBusy: return "bridge_busy";
    case Refusal::kBadSensor: return "bridge_bad_sensor";
    case Refusal::kBadBaud: return "bridge_bad_baud";
    case Refusal::kBadMinutes: return "bridge_bad_minutes";
    case Refusal::kNoSession: return "bridge_no_session";
    case Refusal::kBadToken: return "bridge_bad_token";
    case Refusal::kExpired: return "bridge_expired";
    case Refusal::kTooBig: return "bridge_too_big";
    case Refusal::kTooFast: return "bridge_too_fast";
    case Refusal::kNone: break;
  }
  return "";
}

inline std::string token_text(const std::uint8_t* bytes) {
  static const char hex[] = "0123456789abcdef";
  std::string out;
  for (std::size_t i = 0; i < kTokenBytes; ++i) { out += hex[bytes[i] >> 4]; out += hex[bytes[i] & 15]; }
  return out;
}

// Compares two texts without stopping at the first difference.
inline bool same_secret(const std::string& a, const std::string& b) {
  unsigned diff = a.size() == b.size() ? 0 : 1;
  const std::size_t n = a.size() < b.size() ? a.size() : b.size();
  for (std::size_t i = 0; i < n; ++i) diff |= static_cast<unsigned>(static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]));
  return diff == 0;
}

enum class End : std::uint8_t { kStopped, kExpired, kReplaced };

class Session {
 public:
  bool active() const { return active_; }
  std::size_t sensor() const { return sensor_; }           // 1 to 3
  std::uint32_t baud() const { return baud_; }
  std::uint64_t expires_at_ms() const { return expires_ms_; }
  const std::string& opened_by() const { return by_; }
  std::uint64_t bytes_to_sensor() const { return to_sensor_; }
  std::uint64_t bytes_from_sensor() const { return from_sensor_; }

  // Opens a session. `token_bytes` are random bytes the firmware drew (kTokenBytes of them); the token text is returned in `token`.
  Refusal open(std::size_t sensor, std::uint32_t baud, std::uint32_t minutes, const std::string& by, std::uint64_t now_ms, const std::uint8_t* token_bytes, std::string& token) {
    expire(now_ms);
    if (active_) return Refusal::kBusy;
    if (sensor < 1 || sensor > 3) return Refusal::kBadSensor;
    if (!baud_allowed(baud)) return Refusal::kBadBaud;
    if (minutes == 0 || minutes > kMaxMinutes) return Refusal::kBadMinutes;
    active_ = true;
    sensor_ = sensor;
    baud_ = baud;
    by_ = by.substr(0, 32);
    expires_ms_ = now_ms + static_cast<std::uint64_t>(minutes) * 60000ULL;
    token_ = token_text(token_bytes);
    token = token_;
    window_start_ms_ = now_ms;
    window_bytes_ = 0;
    to_sensor_ = from_sensor_ = 0;
    return Refusal::kNone;
  }

  // Closes the session. True when there was one.
  bool close(End why = End::kStopped) {
    if (!active_) return false;
    active_ = false;
    last_end_ = why;
    token_.clear();
    return true;
  }
  End last_end() const { return last_end_; }

  // Ends a session whose time is up; true when it did.
  bool expire(std::uint64_t now_ms) {
    if (active_ && now_ms >= expires_ms_) { close(End::kExpired); return true; }
    return false;
  }

  // One exchange: `write_bytes` go to the sensor (0 for a poll) and up to `read_bytes` may come back. Refusals leave the session as it was.
  Refusal exchange(const std::string& token, std::size_t write_bytes, std::size_t read_bytes, std::uint64_t now_ms) {
    if (!active_) return expire_note();
    if (now_ms >= expires_ms_) { close(End::kExpired); return Refusal::kExpired; }
    if (!same_secret(token, token_)) return Refusal::kBadToken;
    if (write_bytes > kMaxWrite || read_bytes > kMaxRead) return Refusal::kTooBig;
    if (now_ms - window_start_ms_ >= 1000) { window_start_ms_ = now_ms; window_bytes_ = 0; }
    if (window_bytes_ + write_bytes + read_bytes > kBytesPerSecond) return Refusal::kTooFast;
    window_bytes_ += write_bytes + read_bytes;
    to_sensor_ += write_bytes;
    from_sensor_ += read_bytes;
    return Refusal::kNone;
  }

  // What the sensor really sent back in an exchange that was allowed up to `read_bytes` (the firmware knows only afterwards): keeps the count honest.
  void note_read(std::size_t actually, std::size_t allowed) {
    if (actually < allowed) { from_sensor_ -= (allowed - actually); window_bytes_ -= (allowed - actually); }
  }

 private:
  Refusal expire_note() const { return last_end_ == End::kExpired ? Refusal::kExpired : Refusal::kNoSession; }

  bool active_ = false;
  std::size_t sensor_ = 0;
  std::uint32_t baud_ = 0;
  std::string by_;
  std::string token_;
  std::uint64_t expires_ms_ = 0;
  std::uint64_t window_start_ms_ = 0;
  std::uint64_t window_bytes_ = 0;
  std::uint64_t to_sensor_ = 0, from_sensor_ = 0;
  End last_end_ = End::kStopped;
};

// The record of the history ("what happened" in a fixed vocabulary; the administrator's name is the only free text, cut short).
inline const char* end_text(End why) { return why == End::kExpired ? "expired" : why == End::kReplaced ? "replaced" : "closed"; }

}  // namespace armor::bridge
