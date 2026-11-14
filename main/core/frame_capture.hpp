// ARMOR-NODE-RADAR - the capture of the radar frames: the last frames a radar sent, kept in a small ring in memory and handed to the installer as a text file for tools/frames_to_fixture.py. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Off by default (capture.enabled). While it is off nothing is kept and no memory is held. When it is on the ring holds the last `frames` frames (8 to 200, 60 by default); the oldest one goes when a new
// one comes, and a counter says how many went. The ring never grows past that: it is allocated whole when the capture is turned on and freed when it is turned off.
//
// What goes in: only a frame that has the exact shape of a radar frame (30 bytes, the header aa ff 03 00 and the tail 55 cc). Commands, their answers, the bytes of the transparent serial bridge and anything
// else that crosses the serial lines never match that shape, so no credential and no configuration can end up in the capture; a frame is a handful of target coordinates and nothing else.
// What comes out: one line per frame, "FRAME r<radar> <60 hexadecimal characters>", oldest first: the same lines the build option ARMOR_RADAR_HEX_DUMP writes to the console.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace armor::framecap {

constexpr std::size_t kFrameBytes = 30;
constexpr int kMinFrames = 8;
constexpr int kMaxFrames = 200;
constexpr int kDefaultFrames = 60;

// Does this buffer have the shape of a radar frame (and nothing else)?
inline bool is_frame(const std::uint8_t* bytes, std::size_t length) {
  return bytes != nullptr && length == kFrameBytes && bytes[0] == 0xAA && bytes[1] == 0xFF && bytes[2] == 0x03 && bytes[3] == 0x00 && bytes[kFrameBytes - 2] == 0x55 && bytes[kFrameBytes - 1] == 0xCC;
}

inline int clamp_frames(int frames) { return frames < kMinFrames ? kMinFrames : (frames > kMaxFrames ? kMaxFrames : frames); }

class Capture {
 public:
  // Turns the capture on or off and sets how many frames it keeps. Turning it off, or changing the size, forgets what was kept.
  void configure(bool enabled, int frames) {
    const std::size_t wanted = enabled ? static_cast<std::size_t>(clamp_frames(frames)) : 0;
    if (wanted == slots_.size()) return;
    slots_.clear();
    slots_.shrink_to_fit();
    slots_.resize(wanted);
    next_ = 0;
    count_ = 0;
    dropped_ = 0;
  }
  bool enabled() const { return !slots_.empty(); }
  std::size_t capacity() const { return slots_.size(); }
  std::size_t size() const { return count_; }
  std::uint32_t dropped() const { return dropped_; }

  // Keeps one frame of radar 1 to 3. Returns false when the capture is off or what came is not a frame.
  bool push(unsigned radar, const std::uint8_t* bytes, std::size_t length) {
    if (slots_.empty() || radar < 1 || radar > 3 || !is_frame(bytes, length)) return false;
    Slot& slot = slots_[next_];
    slot.radar = static_cast<std::uint8_t>(radar);
    for (std::size_t i = 0; i < kFrameBytes; ++i) slot.bytes[i] = bytes[i];
    next_ = (next_ + 1) % slots_.size();
    if (count_ < slots_.size()) ++count_; else ++dropped_;
    return true;
  }

  void clear() { next_ = 0; count_ = 0; dropped_ = 0; }

  // The kept frames, oldest first, one "FRAME r<radar> <hex>" line each (the text tools/frames_to_fixture.py reads).
  std::string text() const {
    static const char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(count_ * (kFrameBytes * 2 + 10));
    const std::size_t start = count_ < slots_.size() ? 0 : next_;
    for (std::size_t n = 0; n < count_; ++n) {
      const Slot& slot = slots_[(start + n) % slots_.size()];
      out += "FRAME r";
      out += static_cast<char>('0' + slot.radar);
      out += ' ';
      for (std::size_t i = 0; i < kFrameBytes; ++i) { out += digits[slot.bytes[i] >> 4]; out += digits[slot.bytes[i] & 15]; }
      out += '\n';
    }
    return out;
  }

 private:
  struct Slot {
    std::uint8_t radar = 0;
    std::uint8_t bytes[kFrameBytes] = {};
  };
  std::vector<Slot> slots_;
  std::size_t next_ = 0, count_ = 0;
  std::uint32_t dropped_ = 0;
};

}  // namespace armor::framecap
