// ARMOR-RADAR - the messages a node sends about what its perimeter engine and its sensors found, and how often it sends the targets. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// An event is one short message to armor/radar/<node>/events (contract radar_event.schema.json in ARMOR-COMMON): the words are a fixed list and there is no free text, so nothing secret can get in.
// Every event carries the number of the start (`boot`, random at each start) and a sequence number inside it, so a server that receives the same event twice (one that was sent from the node's
// memory of an outage and also live) knows it is the same one.
#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>

#include "../radar_tracks.hpp"
#include "json.hpp"
#include "node_id.hpp"
#include "perimeter.hpp"

namespace armor::radarevent {

enum class Kind : std::uint8_t {
  kZoneOccupied, kZoneClear, kLoiter, kTripwire, kPathViolation, kMasking, kMaskingEnd,
  kSensorBluetoothOpen, kSensorBluetoothClosed, kWifiJam, kWifiJamEnd, kBridgeOpened, kBridgeClosed,
};

inline const char* kind_text(Kind k) {
  switch (k) {
    case Kind::kZoneOccupied: return "zone_occupied";
    case Kind::kZoneClear: return "zone_clear";
    case Kind::kLoiter: return "loiter";
    case Kind::kTripwire: return "tripwire";
    case Kind::kPathViolation: return "path_violation";
    case Kind::kMasking: return "masking";
    case Kind::kMaskingEnd: return "masking_end";
    case Kind::kSensorBluetoothOpen: return "sensor_bluetooth_open";
    case Kind::kSensorBluetoothClosed: return "sensor_bluetooth_closed";
    case Kind::kWifiJam: return "wifi_jam";
    case Kind::kWifiJamEnd: return "wifi_jam_end";
    case Kind::kBridgeOpened: return "bridge_opened";
    case Kind::kBridgeClosed: return "bridge_closed";
  }
  return "";
}

inline Kind kind_of(perimeter::Event::Type type) {
  switch (type) {
    case perimeter::Event::Type::kZoneOccupied: return Kind::kZoneOccupied;
    case perimeter::Event::Type::kZoneClear: return Kind::kZoneClear;
    case perimeter::Event::Type::kLoiter: return Kind::kLoiter;
    case perimeter::Event::Type::kTripwire: return Kind::kTripwire;
    case perimeter::Event::Type::kPathViolation: return Kind::kPathViolation;
    case perimeter::Event::Type::kMasking: return Kind::kMasking;
    case perimeter::Event::Type::kMaskingEnd: return Kind::kMaskingEnd;
  }
  return Kind::kZoneOccupied;
}

inline std::string topic(const std::string& node_id) { return node_id_is_valid(node_id) ? "armor/radar/" + node_id + "/events" : ""; }

struct Event {
  Kind kind = Kind::kZoneOccupied;
  std::uint64_t timestamp_ms = 0;
  std::string name;                 // the zone or the line (or the sensor's model, for a Bluetooth event)
  int sensor_id = 0;                // 1 to 3, 0: none
  int track_id = 0;
  const char* movement = nullptr;   // still, walking or running, when it is about a target
  int value = -1;                   // seconds, minutes, or the number counted; -1: none
  int count = -1;
  const char* direction = nullptr;  // "in" or "out" (tripwire)
  std::string from;                 // path violation: the zone the target came from, when it came from one
  std::string by;                   // bridge: the administrator who opened it (letters, digits, '.', '_' and '-' only; anything else is left out)
};

inline bool by_is_valid(const std::string& by) {
  if (by.empty() || by.size() > 32) return false;
  for (char c : by) if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return false;
  return true;
}

class Sequence {
 public:
  explicit Sequence(std::uint32_t boot) : boot_(boot) {}
  std::uint32_t boot() const { return boot_; }
  std::uint32_t next() { return ++seq_; }

 private:
  std::uint32_t boot_;
  std::uint32_t seq_ = 0;
};

inline std::string to_json(const std::string& node_id, std::uint32_t boot, std::uint32_t seq, const Event& e) {
  json::Writer w;
  w.begin_object().field("kind", "radar_event").field("node_id", node_id).key("timestamp_ms").integer(static_cast<long long>(e.timestamp_ms));
  w.key("boot").integer(static_cast<long long>(boot)).key("seq").integer(static_cast<long long>(seq)).field("event", kind_text(e.kind));
  if (!e.name.empty()) w.field("name", e.name.substr(0, 24));
  if (e.sensor_id >= 1 && e.sensor_id <= 3) w.field("sensor_id", e.sensor_id);
  if (e.track_id >= 1 && e.track_id <= 255) w.field("track_id", e.track_id);
  if (e.movement != nullptr) w.field("movement", e.movement);
  if (e.value >= 0) w.field("value", e.value);
  if (e.count >= 0) w.field("count", e.count);
  if (e.direction != nullptr) w.field("direction", e.direction);
  if (!e.from.empty()) w.field("from", e.from.substr(0, 24));
  if (by_is_valid(e.by)) w.field("by", e.by);
  w.end_object();
  return w.str();
}

// ---- how often the targets are sent --------------------------------------------------------------------------------------------------------

// Decides whether this telemetry message is sent, for the stream setting. "live": every one. "change": when the set of targets changed enough (another number, another identity, or a target that moved
// more than `move_mm`) and in any case every two seconds. "events": every ten seconds. The targets that are sent are always the real ones (never an empty list because nothing was sent).
class StreamGate {
 public:
  static constexpr std::uint64_t kChangeKeepaliveMs = 2000;
  static constexpr std::uint64_t kEventsKeepaliveMs = 10000;

  bool should_send(perimeter::Stream mode, const Track* tracks, std::size_t count, std::uint64_t now_ms, int move_mm = 150) {
    bool send = mode == perimeter::Stream::kLive;
    if (mode == perimeter::Stream::kOnChange) send = !sent_before_ || now_ms - last_ms_ >= kChangeKeepaliveMs || changed(tracks, count, move_mm);
    if (mode == perimeter::Stream::kEventsOnly) send = !sent_before_ || now_ms - last_ms_ >= kEventsKeepaliveMs;
    if (send) remember(tracks, count, now_ms);
    return send;
  }

 private:
  bool changed(const Track* tracks, std::size_t count, int move_mm) const {
    if (count != last_count_) return true;
    for (std::size_t i = 0; i < count; ++i) {
      bool found = false;
      for (std::size_t j = 0; j < last_count_; ++j) {
        if (last_[j].sensor_id != tracks[i].sensor_id || last_[j].track_id != tracks[i].track_id) continue;
        found = true;
        if (std::abs(last_[j].x_mm - tracks[i].x_mm) > move_mm || std::abs(last_[j].y_mm - tracks[i].y_mm) > move_mm) return true;
      }
      if (!found) return true;
    }
    return false;
  }
  void remember(const Track* tracks, std::size_t count, std::uint64_t now_ms) {
    last_count_ = count > kMaximumTracks ? kMaximumTracks : count;
    for (std::size_t i = 0; i < last_count_; ++i) last_[i] = tracks[i];
    last_ms_ = now_ms;
    sent_before_ = true;
  }
  Track last_[kMaximumTracks]{};
  std::size_t last_count_ = 0;
  std::uint64_t last_ms_ = 0;
  bool sent_before_ = false;
};

}  // namespace armor::radarevent
