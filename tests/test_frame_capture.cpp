// ARMOR-NODE-RADAR - host tests of the capture of the radar frames (main/core/frame_capture.hpp) and of its settings: the ring is bounded, off by default, keeps only radar frames, and its text is what
// tools/frames_to_fixture.py reads. No radar was used. Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <string>

#include "../main/core/frame_capture.hpp"
#include "../main/core/node_config.hpp"

static int failures = 0;
static int checks = 0;
#define CHECK(condition)                                                              \
  do {                                                                                \
    ++checks;                                                                         \
    if (!(condition)) {                                                               \
      ++failures;                                                                     \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);                \
    }                                                                                 \
  } while (0)

using namespace armor;

// A well-formed frame whose first target X byte is `tag`, so frames can be told apart.
static void make_frame(std::uint8_t* out, std::uint8_t tag) {
  for (std::size_t i = 0; i < framecap::kFrameBytes; ++i) out[i] = 0;
  out[0] = 0xAA; out[1] = 0xFF; out[2] = 0x03; out[3] = 0x00;
  out[4] = tag;
  out[28] = 0x55; out[29] = 0xCC;
}

static void test_shape() {
  std::uint8_t frame[framecap::kFrameBytes];
  make_frame(frame, 1);
  CHECK(framecap::is_frame(frame, sizeof frame));
  CHECK(!framecap::is_frame(nullptr, framecap::kFrameBytes));
  CHECK(!framecap::is_frame(frame, framecap::kFrameBytes - 1) && !framecap::is_frame(frame, framecap::kFrameBytes + 1));
  std::uint8_t bad[framecap::kFrameBytes];
  make_frame(bad, 1); bad[0] = 0xAB;
  CHECK(!framecap::is_frame(bad, sizeof bad));
  make_frame(bad, 1); bad[29] = 0xCD;
  CHECK(!framecap::is_frame(bad, sizeof bad));
  make_frame(bad, 1); bad[2] = 0x04;
  CHECK(!framecap::is_frame(bad, sizeof bad));
  CHECK(framecap::clamp_frames(0) == framecap::kMinFrames && framecap::clamp_frames(1000) == framecap::kMaxFrames && framecap::clamp_frames(60) == 60);
}

static void test_ring() {
  framecap::Capture capture;
  std::uint8_t frame[framecap::kFrameBytes];
  make_frame(frame, 1);
  // off by default: nothing is kept and nothing is allocated
  CHECK(!capture.enabled() && capture.capacity() == 0 && capture.size() == 0 && capture.text().empty());
  CHECK(!capture.push(1, frame, sizeof frame) && capture.size() == 0);
  // on: bounded to what was asked (8 at least), refuses what is not a frame or not a radar of this node
  capture.configure(true, 3);
  CHECK(capture.enabled() && capture.capacity() == static_cast<std::size_t>(framecap::kMinFrames));
  capture.configure(true, 9999);
  CHECK(capture.capacity() == static_cast<std::size_t>(framecap::kMaxFrames));
  capture.configure(true, 10);
  CHECK(capture.capacity() == 10);
  CHECK(!capture.push(0, frame, sizeof frame) && !capture.push(4, frame, sizeof frame));
  std::uint8_t command[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03};
  CHECK(!capture.push(1, command, sizeof command) && capture.size() == 0);
  // the oldest goes when the ring is full, and the text is oldest first
  for (std::uint8_t n = 1; n <= 13; ++n) {
    make_frame(frame, n);
    CHECK(capture.push(1 + n % 3, frame, sizeof frame));
  }
  CHECK(capture.size() == 10 && capture.dropped() == 3);
  const std::string text = capture.text();
  std::size_t lines = 0;
  for (char c : text) if (c == '\n') ++lines;
  CHECK(lines == 10);
  CHECK(text.find("FRAME r2 aaff030004") == 0);   // frame 4 is now the oldest (4 % 3 == 1 -> radar 2)
  CHECK(text.find("aaff0300" "03" "000000") == std::string::npos);   // frame 3 went
  CHECK(text.find("aaff0300" "0d" "000000") != std::string::npos);   // frame 13 is there
  CHECK(text.size() == 10 * (std::string("FRAME r1 ").size() + framecap::kFrameBytes * 2 + 1));
  // every line has the form tools/frames_to_fixture.py reads: FRAME r<1-3> <60 hex characters starting aaff0300 and ending 55cc>
  std::size_t at = 0;
  bool every_line_ok = true;
  while (at < text.size()) {
    const std::size_t end = text.find('\n', at);
    const std::string line = text.substr(at, end - at);
    if (line.size() != 9 + 60 || line.compare(0, 7, "FRAME r") != 0 || line[7] < '1' || line[7] > '3' || line[8] != ' ' || line.compare(9, 8, "aaff0300") != 0 || line.compare(9 + 56, 4, "55cc") != 0) every_line_ok = false;
    at = end + 1;
  }
  CHECK(every_line_ok);
  // clearing forgets the frames but keeps the size
  capture.clear();
  CHECK(capture.size() == 0 && capture.dropped() == 0 && capture.capacity() == 10 && capture.text().empty());
  // a ring that is not full is read from its start
  make_frame(frame, 7);
  capture.push(3, frame, sizeof frame);
  CHECK(capture.text().find("FRAME r3 aaff030007") == 0 && capture.size() == 1);
  // the same size again keeps the frames; another size, or turning it off, forgets them and frees the memory
  capture.configure(true, 10);
  CHECK(capture.size() == 1);
  capture.configure(true, 20);
  CHECK(capture.size() == 0 && capture.capacity() == 20);
  capture.configure(false, 20);
  CHECK(!capture.enabled() && capture.capacity() == 0 && !capture.push(1, frame, sizeof frame));
}

static void test_settings() {
  const config::Settings base = config::default_settings("a1b2c3");
  // off by default
  CHECK(!base.capture_enabled && base.capture_frames == framecap::kDefaultFrames);
  // the values load (this comes BEFORE the refusals: a refusal test is worthless if the same document with good values does not load)
  config::Settings out;
  config::Problems problems;
  CHECK(config::load(R"({"capture":{"enabled":true,"frames":120}})", base, out, problems) && problems.empty());
  CHECK(out.capture_enabled && out.capture_frames == 120);
  // the stored form reads back the same (round trip load -> to_json -> load)
  const std::string stored = config::to_json(out, true);
  CHECK(stored.find("\"capture\":{\"enabled\":true,\"frames\":120}") != std::string::npos);
  config::Settings back;
  problems.clear();
  CHECK(config::load(stored, config::default_settings("000000"), back, problems) && back.capture_enabled && back.capture_frames == 120 && config::to_json(back, true) == stored);
  // the defaults are written as they are
  CHECK(config::to_json(base, true).find("\"capture\":{\"enabled\":false,\"frames\":60}") != std::string::npos);
  // mistakes are refused with the path
  const auto refuses = [&](const char* text, const char* path) {
    config::Problems p;
    config::Settings o;
    if (config::load(text, base, o, p)) return false;
    for (const config::Problem& item : p) if (item.path == path) return true;
    return false;
  };
  CHECK(refuses(R"({"capture":{"frames":7}})", "capture.frames"));
  CHECK(refuses(R"({"capture":{"frames":201}})", "capture.frames"));
  CHECK(refuses(R"({"capture":{"frames":"many"}})", "capture.frames"));
  CHECK(refuses(R"({"capture":{"enabled":"yes"}})", "capture.enabled"));
  // it applies at once: turning it on or changing its size needs no restart; another setting still does
  config::Settings changed = out;
  changed.capture_enabled = false;
  changed.capture_frames = 8;
  CHECK(!config::needs_restart(out, changed));
  changed.mqtt.heartbeat_s += 1;
  CHECK(config::needs_restart(out, changed));
}

int main() {
  test_shape();
  test_ring();
  test_settings();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
