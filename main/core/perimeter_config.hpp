// ARMOR-RADAR - the settings of the node's own perimeter engine: zones, tripwire lines, entry paths and the rules around them. Pure.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The server keeps judging the targets of every node (its dwell time, its ignore zones, its profiles); this engine is the part that runs on the node itself, next to the sensors: it
// still sees a perimeter when the broker cannot be reached, it counts what crosses a line, and it tells the server what happened in a few words (events) instead of a stream of targets.
// It is OFF by default and, when it is on, every rule that can raise an alarm is off or lenient until it is set (no zones, no lines, no paths, no masking, no pet immunity).
//
// Coordinates are millimetres on the node's shared plane (the plane the radar calibration puts all the sensors on: x sideways, y forward). A zone is the rectangle that starts at the corner (x, y)
// and extends width to the right and height forward, then turned by `angle` degrees (counter-clockwise) around that corner: a zone across a corner of a yard is a rectangle turned 45 degrees.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace armor::perimeter {

constexpr std::size_t kMaxDetectZones = 6;
constexpr std::size_t kMaxExcludeZones = 3;
constexpr std::size_t kMaxZones = kMaxDetectZones + kMaxExcludeZones;
constexpr std::size_t kMaxLines = 3;
constexpr std::size_t kMaxPaths = 3;
constexpr std::size_t kPathLength = 4;
constexpr std::size_t kMaxName = 16;

constexpr int kProfileDay = 1;
constexpr int kProfileNight = 2;
constexpr int kProfileAlways = 3;   // a bit mask: day = 1, night = 2

enum class Stream : std::uint8_t {
  kLive,         // every telemetry message carries the targets (as before)
  kOnChange,     // a message when the targets changed enough, and at least every two seconds
  kEventsOnly,   // the targets are told every ten seconds; what happens in between is told by events
};

struct Zone {
  bool enabled = false;
  std::string name;              // a-z, 0-9 and '_'; the name the commands and the events use (and the name of the zone as a device of the server)
  bool exclude = false;          // an exclusion zone: what is inside it is ignored by everything below (a window, a fan, a gate that opens)
  int x_mm = 0, y_mm = 0;        // the corner the rectangle starts from
  int width_mm = 0, height_mm = 0;
  int angle_deg = 0;             // -180..180, turned around the corner
  int profiles = kProfileAlways; // when the zone is on: 1 by day, 2 by night, 3 always
  bool latch = false;            // "the target must leave": the zone stays occupied after the sensor loses a target inside it, until it is seen leaving
  int loiter_s = 0;              // 0: no loitering rule; otherwise somebody inside for this long is loitering
  bool active = true;            // starting state of the zone; a command changes it while the node runs
};

struct Line {
  bool enabled = false;
  std::string name;
  int ax_mm = 0, ay_mm = 0, bx_mm = 0, by_mm = 0;
};

struct Path {
  bool enabled = false;
  std::string name;
  std::array<std::string, kPathLength> zones;   // names of zones, in order along the way in; an empty name ends the list
};

struct Config {
  bool enabled = false;
  int range_min_mm = 0;          // -500..4000: nothing nearer to the sensors than this is looked at
  int range_max_mm = 8000;       // 1000..8000
  int debounce_frames = 2;       // 1..20: a target must be inside a zone for this many frames before the zone is occupied (and outside as many before it is left)
  int count_window = 5;          // 1..20: the number of targets is the median of this many frames (a radar often says 1, 2, 1 for a single person)
  std::array<Zone, kMaxZones> zones;
  std::array<Line, kMaxLines> lines;
  std::array<Path, kMaxPaths> paths;
  bool strict_entry = false;     // with paths: a target that appears inside a zone of a path other than at its ends is a violation too
  bool night_enabled = false;    // zones have day and night profiles
  int night_from_min = 22 * 60;  // minutes after midnight, local time
  int night_to_min = 6 * 60;
  bool pet_immunity = false;     // presence sensors that report energy: a small, weak target near the sensor is not somebody
  int pet_distance_cm = 200;     // 50..400
  int pet_energy = 30;           // 1..100: moving and stationary energy both at or under this
  int masking_minutes = 0;       // 0: off. Minutes without any target, while armed, before the sensors are suspected to be covered
  bool masking_armed_only = true;
  bool jam_detection = false;    // the Wi-Fi signal collapsing together with the network (a jammer) is told
  Stream stream = Stream::kLive;
};

inline bool name_is_valid(const std::string& name) {
  if (name.empty() || name.size() > kMaxName) return false;
  for (char c : name) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
  return true;
}

inline const char* stream_text(Stream s) { return s == Stream::kOnChange ? "change" : s == Stream::kEventsOnly ? "events" : "live"; }

}  // namespace armor::perimeter
