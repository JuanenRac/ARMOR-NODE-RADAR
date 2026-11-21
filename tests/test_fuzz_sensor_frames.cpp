// ARMOR-NODE-RADAR - host tests with a fixed seed: the readers of what the radar modules send (the LD2450 frames and the LD2461 frames, and the framers that cut them out of the serial stream) must
// never crash, never accept a damaged frame and never turn a value no real module could report into a track, whatever arrives on the serial line.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "../main/core/frame_framer.hpp"
#include "../main/core/ld2450.hpp"
#include "../main/core/ld2461.hpp"
#include "../main/core/var_framer.hpp"

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

// the same numbers on every run, so a failure can be repeated
static std::uint32_t rng_state = 0x3C6EF372u;
static std::uint32_t next_random() {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return rng_state;
}
static std::uint32_t below(std::uint32_t limit) { return limit == 0 ? 0 : next_random() % limit; }
static int between(int low, int high) { return low + static_cast<int>(below(static_cast<std::uint32_t>(high - low + 1))); }

// bytes that are never `avoid`, so that a stream of them cannot start a frame
static std::vector<std::uint8_t> noise_without(std::size_t length, std::uint8_t avoid) {
  std::vector<std::uint8_t> out;
  for (std::size_t i = 0; i < length; ++i) {
    std::uint8_t byte = static_cast<std::uint8_t>(below(256));
    if (byte == avoid) byte = static_cast<std::uint8_t>(avoid + 1);
    out.push_back(byte);
  }
  return out;
}

static std::vector<std::uint8_t> noise(std::size_t length) {
  std::vector<std::uint8_t> out;
  for (std::size_t i = 0; i < length; ++i) out.push_back(static_cast<std::uint8_t>(below(256)));
  return out;
}

using namespace armor;

// ---- LD2450 ---------------------------------------------------------------------------------------------------------------------------------

static void put_signed(std::vector<std::uint8_t>& out, int value) {   // bit 15 set means positive, the other fifteen bits are the size
  const unsigned raw = value >= 0 ? (0x8000u | static_cast<unsigned>(value)) : static_cast<unsigned>(-value);
  out.push_back(static_cast<std::uint8_t>(raw & 0xFF));
  out.push_back(static_cast<std::uint8_t>(raw >> 8));
}

static std::vector<std::uint8_t> ld2450_frame(const int (&x)[3], const int (&y)[3], const int (&speed)[3], const unsigned (&resolution)[3]) {
  std::vector<std::uint8_t> frame(ld2450::kHeader, ld2450::kHeader + ld2450::kHeaderLength);
  for (int slot = 0; slot < 3; ++slot) {
    put_signed(frame, x[slot]);
    put_signed(frame, y[slot]);
    put_signed(frame, speed[slot]);
    frame.push_back(static_cast<std::uint8_t>(resolution[slot] & 0xFF));
    frame.push_back(static_cast<std::uint8_t>(resolution[slot] >> 8));
  }
  frame.insert(frame.end(), ld2450::kTail, ld2450::kTail + ld2450::kTailLength);
  return frame;
}

static std::vector<std::uint8_t> plausible_ld2450() {
  int x[3], y[3], speed[3];
  unsigned resolution[3];
  for (int slot = 0; slot < 3; ++slot) {
    x[slot] = between(-ld2450::kMaxAbsXMm, ld2450::kMaxAbsXMm);
    y[slot] = between(1, ld2450::kMaxYMm);
    speed[slot] = between(-ld2450::kMaxAbsSpeedCmS, ld2450::kMaxAbsSpeedCmS);
    resolution[slot] = 1 + below(400);
  }
  return ld2450_frame(x, y, speed, resolution);
}

static void ld2450_known_frames() {
  for (int round = 0; round < 1000; ++round) {
    int x[3], y[3], speed[3];
    unsigned resolution[3];
    for (int slot = 0; slot < 3; ++slot) {
      x[slot] = between(-ld2450::kMaxAbsXMm, ld2450::kMaxAbsXMm);
      y[slot] = between(1, ld2450::kMaxYMm);
      speed[slot] = between(-ld2450::kMaxAbsSpeedCmS, ld2450::kMaxAbsSpeedCmS);
      resolution[slot] = 1 + below(400);
    }
    const std::vector<std::uint8_t> bytes = ld2450_frame(x, y, speed, resolution);
    CHECK(bytes.size() == ld2450::kFrameLength);
    ld2450::Frame frame;
    CHECK(ld2450::decode_frame(bytes.data(), bytes.size(), frame));
    for (int slot = 0; slot < 3; ++slot) {
      const ld2450::Target& target = frame.targets[slot];
      CHECK(target.present && target.valid && target.rejected == ld2450::Reject::kNone);
      CHECK(target.x_mm == x[slot] && target.y_mm == y[slot] && target.speed_cm_s == speed[slot] && target.resolution_mm == resolution[slot]);
    }
    Track tracks[ld2450::kTargetsPerFrame] = {};
    const std::size_t count = ld2450::to_tracks(2, frame, tracks);
    CHECK(count == 3 && frame.valid_count() == 3 && frame.count() == 3);
    for (std::size_t i = 0; i < count; ++i) CHECK(track_is_valid(tracks[i]) && tracks[i].track_id == i + 1 && tracks[i].sensor_id == 2);
  }
}

static void ld2450_refused_targets() {
  const int zero[3] = {0, 0, 0};
  const unsigned some[3] = {100, 100, 100};
  const unsigned none[3] = {0, 0, 0};
  ld2450::Frame frame;
  // a target the module cannot have measured is not a track: beyond its range, without a resolution, or exactly on the radar
  const int far_x[3] = {ld2450::kMaxAbsXMm + 1, -ld2450::kMaxAbsXMm - 1, 100};
  const int far_y[3] = {1000, 1000, ld2450::kMaxYMm + 1};
  std::vector<std::uint8_t> bytes = ld2450_frame(far_x, far_y, zero, some);
  CHECK(ld2450::decode_frame(bytes.data(), bytes.size(), frame) && frame.count() == 3 && frame.valid_count() == 0);
  for (int slot = 0; slot < 3; ++slot) CHECK(frame.targets[slot].rejected == ld2450::Reject::kOutOfRange);
  const int fast[3] = {100, 100, 100};
  const int too_fast[3] = {ld2450::kMaxAbsSpeedCmS + 1, -ld2450::kMaxAbsSpeedCmS - 1, 0};
  const int near_y[3] = {1000, 1000, ld2450::kMinYMm - 1};
  bytes = ld2450_frame(fast, near_y, too_fast, some);
  CHECK(ld2450::decode_frame(bytes.data(), bytes.size(), frame) && frame.valid_count() == 0);
  bytes = ld2450_frame(fast, near_y, zero, none);
  CHECK(ld2450::decode_frame(bytes.data(), bytes.size(), frame) && frame.valid_count() == 0 && frame.targets[0].rejected == ld2450::Reject::kNoResolution);
  const int origin[3] = {0, 0, 0};
  bytes = ld2450_frame(origin, origin, zero, some);
  CHECK(ld2450::decode_frame(bytes.data(), bytes.size(), frame) && frame.valid_count() == 0 && frame.targets[0].rejected == ld2450::Reject::kOrigin);
  // a frame of empty slots is a frame without targets
  std::vector<std::uint8_t> empty(ld2450::kFrameLength, 0);
  for (std::size_t i = 0; i < ld2450::kHeaderLength; ++i) empty[i] = ld2450::kHeader[i];
  for (std::size_t i = 0; i < ld2450::kTailLength; ++i) empty[ld2450::kFrameLength - ld2450::kTailLength + i] = ld2450::kTail[i];
  CHECK(ld2450::decode_frame(empty.data(), empty.size(), frame) && frame.count() == 0);
}

static void ld2450_damaged_frames() {
  for (int round = 0; round < 500; ++round) {
    const std::vector<std::uint8_t> good = plausible_ld2450();
    ld2450::Frame frame;
    // a frame cut short or longer is not a frame, and says nothing
    const std::size_t cut = below(static_cast<std::uint32_t>(good.size()));
    CHECK(!ld2450::decode_frame(good.data(), cut, frame) && frame.count() == 0);
    std::vector<std::uint8_t> longer = good;
    longer.push_back(0);
    CHECK(!ld2450::decode_frame(longer.data(), longer.size(), frame));
    // a changed byte of the header or of the tail
    std::vector<std::uint8_t> bad = good;
    const std::size_t at = below(2) == 0 ? below(static_cast<std::uint32_t>(ld2450::kHeaderLength)) : ld2450::kFrameLength - ld2450::kTailLength + below(static_cast<std::uint32_t>(ld2450::kTailLength));
    bad[at] = static_cast<std::uint8_t>(bad[at] + 1 + below(255));
    CHECK(!ld2450::decode_frame(bad.data(), bad.size(), frame));
  }
  ld2450::Frame none;
  CHECK(!ld2450::decode_frame(nullptr, 0, none) && none.count() == 0);
}

static void ld2450_noise() {
  // noise inside a frame with the right head and tail: only the targets can be refused, and what is a track is something the module could report
  for (int round = 0; round < 6000; ++round) {
    std::vector<std::uint8_t> bytes = noise(ld2450::kFrameLength);
    for (std::size_t i = 0; i < ld2450::kHeaderLength; ++i) bytes[i] = ld2450::kHeader[i];
    for (std::size_t i = 0; i < ld2450::kTailLength; ++i) bytes[ld2450::kFrameLength - ld2450::kTailLength + i] = ld2450::kTail[i];
    if (below(3) == 0) for (std::size_t i = ld2450::kHeaderLength; i < ld2450::kHeaderLength + 8; ++i) bytes[i] = 0;   // an empty slot among the others
    ld2450::Frame frame;
    CHECK(ld2450::decode_frame(bytes.data(), bytes.size(), frame));
    Track tracks[ld2450::kTargetsPerFrame] = {};
    const std::size_t count = ld2450::to_tracks(1 + below(3), frame, tracks);
    CHECK(count == frame.valid_count() && count <= ld2450::kTargetsPerFrame);
    for (std::size_t i = 0; i < count; ++i) {
      CHECK(track_is_valid(tracks[i]));
      CHECK(tracks[i].x_mm >= -ld2450::kMaxAbsXMm && tracks[i].x_mm <= ld2450::kMaxAbsXMm && tracks[i].y_mm >= ld2450::kMinYMm && tracks[i].y_mm <= ld2450::kMaxYMm);
      CHECK(tracks[i].speed_mm_s >= -ld2450::kMaxAbsSpeedCmS * 10 && tracks[i].speed_mm_s <= ld2450::kMaxAbsSpeedCmS * 10);
    }
    // whatever the length, the same answer for anything that is not a frame
    ld2450::Frame other;
    (void)ld2450::decode_frame(bytes.data(), below(60), other);
  }
}

static void ld2450_stream() {
  // whole frames in a stream of noise that cannot start one, cut in pieces of any size: every frame comes out, once
  for (int round = 0; round < 200; ++round) {
    FrameFramer framer(ld2450::protocol());
    CHECK(framer.configured());
    std::vector<std::uint8_t> stream;
    std::size_t sent = 0;
    for (std::uint32_t i = 1 + below(8); i > 0; --i) {
      const std::vector<std::uint8_t> junk = noise_without(below(20), 0xAA);
      stream.insert(stream.end(), junk.begin(), junk.end());
      const std::vector<std::uint8_t> frame = plausible_ld2450();
      stream.insert(stream.end(), frame.begin(), frame.end());
      ++sent;
    }
    std::size_t got = 0;
    const auto handle = [&](const std::uint8_t* frame, std::size_t length) {
      ld2450::Frame decoded;
      CHECK(ld2450::decode_frame(frame, length, decoded) && decoded.valid_count() == 3);
      ++got;
    };
    for (std::size_t at = 0; at < stream.size();) {
      const std::size_t piece = 1 + below(40);
      const std::size_t take = at + piece > stream.size() ? stream.size() - at : piece;
      framer.feed(stream.data() + at, take, handle);
      at += take;
    }
    CHECK(got == sent && framer.stats().frames == sent && framer.stats().bad_frames == 0);
  }
  // a stream of anything: no crash, every frame handed over is a whole frame of the protocol
  FrameFramer framer(ld2450::protocol());
  std::size_t handed = 0;
  const auto check_frame = [&](const std::uint8_t* frame, std::size_t length) {
    ld2450::Frame decoded;
    CHECK(length == ld2450::kFrameLength && ld2450::decode_frame(frame, length, decoded));
    ++handed;
  };
  for (int round = 0; round < 400; ++round) {
    std::vector<std::uint8_t> junk = noise(below(300));
    if (below(2) == 0) { const std::vector<std::uint8_t> frame = plausible_ld2450(); junk.insert(junk.begin() + static_cast<std::ptrdiff_t>(below(static_cast<std::uint32_t>(junk.size() + 1))), frame.begin(), frame.end()); }
    framer.feed(junk.data(), junk.size(), check_frame);
  }
  CHECK(handed > 0);
  // the framer that was not given a protocol takes nothing
  FrameFramer unconfigured(ProtocolSpec::unconfigured());
  const std::vector<std::uint8_t> frame = plausible_ld2450();
  unconfigured.feed(frame.data(), frame.size(), [&](const std::uint8_t*, std::size_t) { CHECK(false); });
  CHECK(!unconfigured.configured());
}

static void track_memory() {
  // the radars' latest tracks: never more than the contract has room for, never a track of a radar that went quiet
  TrackSet set(500);
  std::uint64_t now = 1000;
  for (int round = 0; round < 3000; ++round) {
    now += below(400);
    Track tracks[10] = {};
    const std::size_t count = below(11);
    for (std::size_t i = 0; i < count; ++i) tracks[i] = Track{static_cast<std::uint8_t>(below(5)), static_cast<std::uint8_t>(below(4)), static_cast<std::int16_t>(below(65536)), static_cast<std::int16_t>(below(65536)), 0};
    set.update(static_cast<std::uint8_t>(below(5)), tracks, count, now);
    if (below(7) == 0) set.set_max_age(static_cast<std::uint8_t>(below(5)), below(3000));
    Track out[kMaximumTracks] = {};
    const std::size_t total = set.collect(out, now + below(800));
    CHECK(total <= kMaximumTracks);
  }
  Track out[kMaximumTracks] = {};
  CHECK(set.collect(out, now + 1'000'000) == 0 && !set.any_fresh(now + 1'000'000));
}

// ---- LD2461 ---------------------------------------------------------------------------------------------------------------------------------

static std::vector<std::uint8_t> ld2461_frame(std::uint8_t command, const std::vector<std::uint8_t>& value) {
  std::uint8_t out[3 + 2 + 1 + ld2461::kMaxValue + 1 + 3];
  const std::size_t length = ld2461::build(command, value.data(), value.size(), out, sizeof out);
  return std::vector<std::uint8_t>(out, out + length);
}

static std::vector<std::uint8_t> value_of(std::size_t length) {
  std::vector<std::uint8_t> out;
  for (std::size_t i = 0; i < length; ++i) out.push_back(static_cast<std::uint8_t>(below(256)));
  return out;
}

static void ld2461_built_frames() {
  for (int round = 0; round < 2000; ++round) {
    const std::uint8_t command = static_cast<std::uint8_t>(below(256));
    const std::vector<std::uint8_t> value = value_of(below(ld2461::kMaxValue + 1));
    const std::vector<std::uint8_t> bytes = ld2461_frame(command, value);
    CHECK(bytes.size() == 10 + value.size());
    ld2461::Frame frame;
    CHECK(ld2461::parse(bytes.data(), bytes.size(), frame) && frame.command == command && frame.value_length == value.size());
    for (std::size_t i = 0; i < value.size(); ++i) CHECK(frame.value[i] == value[i]);
    // one changed byte, anywhere, is seen: the head, the length, the sum and the end are all checked
    std::vector<std::uint8_t> bad = bytes;
    const std::size_t at = below(static_cast<std::uint32_t>(bad.size()));
    bad[at] = static_cast<std::uint8_t>(bad[at] + 1 + below(255));
    CHECK(!ld2461::parse(bad.data(), bad.size(), frame));
    // cut short or longer
    CHECK(!ld2461::parse(bytes.data(), below(static_cast<std::uint32_t>(bytes.size())), frame));
    std::vector<std::uint8_t> longer = bytes;
    longer.push_back(0);
    CHECK(!ld2461::parse(longer.data(), longer.size(), frame));
  }
  // a value that does not fit, or a buffer that is too small, builds nothing
  std::uint8_t out[16];
  const std::vector<std::uint8_t> big = value_of(ld2461::kMaxValue + 1);
  std::uint8_t large[128];
  CHECK(ld2461::build(1, big.data(), big.size(), large, sizeof large) == 0);
  CHECK(ld2461::build(1, big.data(), 4, out, 5) == 0);
  CHECK(ld2461::build_set_format(0, out, sizeof out) == 0 && ld2461::build_set_format(4, out, sizeof out) == 0 && ld2461::build_set_format(3, out, sizeof out) > 0);
  CHECK(ld2461::build_cancel_zone(0, out, sizeof out) == 0 && ld2461::build_cancel_zone(4, out, sizeof out) == 0 && ld2461::build_cancel_zone(2, out, sizeof out) > 0);
  CHECK(ld2461::build_baud(12345, out, sizeof out) == 0 && ld2461::build_baud(115200, out, sizeof out) > 0);
  std::uint8_t zone[32];
  for (int round = 0; round < 1000; ++round) {
    const std::size_t length = ld2461::build_set_zone(static_cast<std::uint8_t>(below(6)), static_cast<std::uint8_t>(below(3)), between(-15000, 15000), between(-15000, 15000), between(-15000, 15000), between(-15000, 15000), zone, sizeof zone);
    ld2461::Frame frame;
    if (length != 0) CHECK(ld2461::parse(zone, length, frame) && frame.command == ld2461::kSetZone && frame.value_length == 10);
  }
}

static void ld2461_noise() {
  for (int round = 0; round < 6000; ++round) {
    // noise that has the head, the length, the sum and the end: only the content can be refused
    const std::vector<std::uint8_t> bytes = ld2461_frame(below(3) == 0 ? static_cast<std::uint8_t>(below(256)) : static_cast<std::uint8_t>(below(3) + 7), value_of(below(ld2461::kMaxValue + 1)));
    ld2461::Frame frame;
    CHECK(ld2461::parse(bytes.data(), bytes.size(), frame));
    Track tracks[ld2461::kMaxTargets] = {};
    const std::size_t count = ld2461::to_tracks(1 + below(3), frame, tracks);
    CHECK(count <= ld2461::kMaxTargets && (frame.command == ld2461::kReportCoordinates || count == 0));
    for (std::size_t i = 0; i < count; ++i) CHECK(track_is_valid(tracks[i]) && tracks[i].x_mm >= -12800 && tracks[i].x_mm <= 12700 && tracks[i].y_mm >= -12800 && tracks[i].y_mm <= 12700);
    bool occupied[3] = {false, false, false};
    CHECK(ld2461::zone_occupancy(frame, occupied) == (frame.command == ld2461::kReportZones && frame.value_length >= 3));
    ld2461::Zone zones[3];
    CHECK(ld2461::zones_from(frame, zones) == (frame.command == ld2461::kReadZones && frame.value_length >= 30));
    (void)ld2461::version_from(frame);
    // and any bytes at all
    const std::vector<std::uint8_t> junk = noise(below(70));
    ld2461::Frame other;
    (void)ld2461::parse(junk.data(), junk.size(), other);
  }
}

static void ld2461_stream() {
  const VarFrameSpec spec = ld2461::protocol();
  CHECK(spec.configured());
  // whole frames in a stream of noise that cannot start one, in pieces of any size
  for (int round = 0; round < 200; ++round) {
    VarFramer framer(spec);
    std::vector<std::uint8_t> stream;
    std::size_t sent = 0;
    for (std::uint32_t i = 1 + below(8); i > 0; --i) {
      const std::vector<std::uint8_t> junk = noise_without(below(20), 0xFF);
      stream.insert(stream.end(), junk.begin(), junk.end());
      const std::vector<std::uint8_t> frame = ld2461_frame(static_cast<std::uint8_t>(below(256)), value_of(below(ld2461::kMaxValue + 1)));
      stream.insert(stream.end(), frame.begin(), frame.end());
      ++sent;
    }
    std::size_t got = 0;
    const auto handle = [&](const std::uint8_t* frame, std::size_t length) {
      ld2461::Frame decoded;
      CHECK(ld2461::parse(frame, length, decoded));
      ++got;
    };
    for (std::size_t at = 0; at < stream.size();) {
      const std::size_t piece = 1 + below(40);
      const std::size_t take = at + piece > stream.size() ? stream.size() - at : piece;
      framer.feed(stream.data() + at, take, handle);
      at += take;
    }
    CHECK(got == sent && framer.stats().frames == sent);
  }
  // a stream of anything: no crash, and every frame handed over has the head, a length that fits and the end
  VarFramer framer(spec);
  const auto check_frame = [&](const std::uint8_t* frame, std::size_t length) {
    CHECK(length >= 9 && length <= spec.max_frame);
    CHECK(frame[0] == 0xFF && frame[1] == 0xEE && frame[2] == 0xDD && frame[length - 3] == 0xDD && frame[length - 2] == 0xEE && frame[length - 1] == 0xFF);
  };
  for (int round = 0; round < 600; ++round) {
    std::vector<std::uint8_t> junk = noise(below(300));
    if (below(2) == 0) { const std::vector<std::uint8_t> frame = ld2461_frame(7, value_of(10)); junk.insert(junk.begin() + static_cast<std::ptrdiff_t>(below(static_cast<std::uint32_t>(junk.size() + 1))), frame.begin(), frame.end()); }
    framer.feed(junk.data(), junk.size(), check_frame);
  }
  // a framer with a protocol that cannot be (a footer longer than what the length leaves) takes nothing
  VarFrameSpec broken = spec;
  broken.length_extra = 1;
  VarFramer refusing(broken);
  const std::vector<std::uint8_t> frame = ld2461_frame(7, value_of(10));
  refusing.feed(frame.data(), frame.size(), [&](const std::uint8_t*, std::size_t) { CHECK(false); });
}

int main() {
  ld2450_known_frames();
  ld2450_refused_targets();
  ld2450_damaged_frames();
  ld2450_noise();
  ld2450_stream();
  track_memory();
  ld2461_built_frames();
  ld2461_noise();
  ld2461_stream();
  std::printf("fuzz_sensor_frames: %d checks, %d failed\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
