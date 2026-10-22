// ARMOR-RADAR - the node's own perimeter engine: what the targets do in the zones, across the lines and along the paths. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Each frame it takes the targets of the node (already on the shared plane, merged where two sensors see one person, each with the stable identity that track_stabiliser.hpp gives) and
// tells, as a short list of events, what changed:
//   - a detection zone becomes occupied or clear (debounced by a number of frames, with the time it was occupied), and may "hold" a target the sensor lost inside it until it is seen leaving;
//   - somebody has been inside a zone longer than its loiter time;
//   - a target crossed a tripwire line, with its direction, and the line counts the ins and the outs;
//   - a target moved between two zones in an order no configured path allows (or appeared in the middle of a path);
//   - the sensors saw no target for the configured minutes while armed (covered?), and saw one again.
// It also classifies every target (still, walking, running) and keeps the median number of targets over a short window (a radar says 1, 2, 1 for one person).
// Targets in an exclusion zone, or outside the configured range, do not exist for any of this. Nothing here touches hardware or the network.
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "../radar_tracks.hpp"
#include "perimeter_config.hpp"

namespace armor::perimeter {

enum class Movement : std::uint8_t { kStill, kWalking, kRunning };
inline const char* movement_text(Movement m) { return m == Movement::kRunning ? "running" : m == Movement::kWalking ? "walking" : "still"; }

// A target's speed class from its (smoothed) speed, with a margin so that a value on the line does not flip every frame.
constexpr int kStillMmS = 150;      // under this: not moving (a person standing still breathes at well under 0.15 m/s)
constexpr int kRunningMmS = 2500;   // from this: running (a brisk walk is about 1.5 m/s)
inline Movement classify(int smoothed_mm_s, Movement before) {
  const int still_up = before == Movement::kStill ? kStillMmS : kStillMmS * 2 / 3;       // leaving "still" takes more than entering it
  const int run_up = before == Movement::kRunning ? kRunningMmS * 85 / 100 : kRunningMmS;
  if (smoothed_mm_s >= run_up) return Movement::kRunning;
  if (smoothed_mm_s > still_up) return Movement::kWalking;
  return Movement::kStill;
}

struct Event {
  enum class Type : std::uint8_t { kZoneOccupied, kZoneClear, kLoiter, kTripwire, kPathViolation, kMasking, kMaskingEnd };
  Type type = Type::kZoneOccupied;
  int index = -1;                 // the zone, the line (or, for a path violation, the zone entered)
  std::uint8_t sensor = 0;        // the target, when the event is about one
  std::uint8_t track = 0;
  int value = 0;                  // clear: seconds it was occupied. loiter: seconds. tripwire: +1 in, -1 out. path violation: the zone it came from (-1: appeared there). masking: minutes
  int value2 = 0;                 // tripwire: how many have crossed this line in this direction since the node started
  Movement movement = Movement::kStill;
  std::uint64_t at_ms = 0;
};

inline const char* event_text(Event::Type type) {
  switch (type) {
    case Event::Type::kZoneOccupied: return "zone_occupied";
    case Event::Type::kZoneClear: return "zone_clear";
    case Event::Type::kLoiter: return "loiter";
    case Event::Type::kTripwire: return "tripwire";
    case Event::Type::kPathViolation: return "path_violation";
    case Event::Type::kMasking: return "masking";
    case Event::Type::kMaskingEnd: return "masking_end";
  }
  return "";
}

// ---- geometry ---------------------------------------------------------------------------------------------------------------------------

namespace geometry {
constexpr double kPi = 3.14159265358979323846;

// Where a point is in the frame of a zone: u along its width, v along its height, both from the corner. Zone sizes are positive.
inline void local(const Zone& zone, int px, int py, double& u, double& v) {
  const double a = static_cast<double>(zone.angle_deg) * kPi / 180.0;
  const double dx = static_cast<double>(px) - zone.x_mm, dy = static_cast<double>(py) - zone.y_mm;
  u = dx * std::cos(a) + dy * std::sin(a);
  v = -dx * std::sin(a) + dy * std::cos(a);
}
inline bool inside(const Zone& zone, int px, int py) {
  if (zone.width_mm <= 0 || zone.height_mm <= 0) return false;
  double u, v;
  local(zone, px, py, u, v);
  return u >= 0 && u <= zone.width_mm && v >= 0 && v <= zone.height_mm;
}
// The distance from a point inside the zone to its nearest edge (negative outside).
inline double edge_distance(const Zone& zone, int px, int py) {
  double u, v;
  local(zone, px, py, u, v);
  const double a = u < zone.width_mm - u ? u : zone.width_mm - u, b = v < zone.height_mm - v ? v : zone.height_mm - v;
  return a < b ? a : b;
}
inline std::int64_t cross(std::int64_t ax, std::int64_t ay, std::int64_t bx, std::int64_t by, std::int64_t cx, std::int64_t cy) { return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax); }
// Whether the segment p1-p2 crosses the segment q1-q2 (touching an end counts).
inline bool segments_cross(std::int64_t p1x, std::int64_t p1y, std::int64_t p2x, std::int64_t p2y, std::int64_t q1x, std::int64_t q1y, std::int64_t q2x, std::int64_t q2y) {
  const auto sign = [](std::int64_t v) { return v > 0 ? 1 : v < 0 ? -1 : 0; };
  const int d1 = sign(cross(q1x, q1y, q2x, q2y, p1x, p1y)), d2 = sign(cross(q1x, q1y, q2x, q2y, p2x, p2y));
  const int d3 = sign(cross(p1x, p1y, p2x, p2y, q1x, q1y)), d4 = sign(cross(p1x, p1y, p2x, p2y, q2x, q2y));
  return d1 * d2 <= 0 && d3 * d4 <= 0;
}
}  // namespace geometry

// ---- the number of targets, without the flicker -----------------------------------------------------------------------------------

// The median of the last `window` counts. A radar that says 1, 2, 1 for one person says 1.
class CountSmoother {
 public:
  void set_window(int window) { window_ = window < 1 ? 1 : window > static_cast<int>(kCapacity) ? static_cast<int>(kCapacity) : static_cast<std::size_t>(window); filled_ = 0; next_ = 0; }
  void push(std::size_t count) {
    values_[next_] = static_cast<std::uint8_t>(count > 255 ? 255 : count);
    next_ = (next_ + 1) % window_;
    if (filled_ < window_) ++filled_;
  }
  std::size_t value() const {
    if (filled_ == 0) return 0;
    std::uint8_t sorted[kCapacity];
    for (std::size_t i = 0; i < filled_; ++i) sorted[i] = values_[i];
    for (std::size_t i = 1; i < filled_; ++i) { const std::uint8_t key = sorted[i]; std::size_t j = i; while (j > 0 && sorted[j - 1] > key) { sorted[j] = sorted[j - 1]; --j; } sorted[j] = key; }
    return sorted[filled_ / 2];
  }

 private:
  static constexpr std::size_t kCapacity = 20;
  std::size_t window_ = 5;
  std::size_t filled_ = 0, next_ = 0;
  std::uint8_t values_[kCapacity]{};
};

// ---- the engine -------------------------------------------------------------------------------------------------------------------------

struct Context {
  std::uint64_t now_ms = 0;
  bool night = false;     // local time is inside the night window
  bool armed = false;     // the server (or the panel) said the perimeter is armed; only the masking rule looks at it
};

struct ZoneState {
  bool occupied = false;
  bool latched = false;
  bool on = false;                 // enabled, switched on, and in its profile now
  unsigned occupants = 0;
  std::uint64_t since_ms = 0;
  std::uint32_t last_dwell_s = 0;
  std::uint32_t entries = 0;
};
struct LineState { std::uint32_t ins = 0, outs = 0; };

class Engine {
 public:
  static constexpr std::uint64_t kLostMs = 1000;            // a target not seen for this long is gone
  static constexpr std::uint64_t kLatchMaxMs = 30ULL * 60 * 1000;
  static constexpr std::uint64_t kPathWindowMs = 3000;
  static constexpr int kLineHysteresisMm = 120;
  static constexpr double kLatchEdgeMm = 400.0;             // a target lost this far inside a zone is assumed to be still in it

  explicit Engine(const Config& config = Config{}) { configure(config); }

  void configure(const Config& config) {
    config_ = config;
    reset();
    for (std::size_t z = 0; z < kMaxZones; ++z) runtime_active_[z] = config_.zones[z].active;
    smoother_.set_window(config_.count_window);
    // paths: the zones they name, in order; the steps between neighbours are allowed both ways
    for (std::size_t a = 0; a < kMaxZones; ++a) { managed_[a] = entry_[a] = false; for (std::size_t b = 0; b < kMaxZones; ++b) allowed_[a][b] = false; }
    any_paths_ = false;
    for (const Path& path : config_.paths) {
      if (!path.enabled) continue;
      int previous = -1, first = -1;
      for (const std::string& name : path.zones) {
        if (name.empty()) break;
        const int z = zone_index(name);
        if (z < 0 || config_.zones[static_cast<std::size_t>(z)].exclude) { previous = -2; break; }
        if (first < 0) first = z;
        managed_[static_cast<std::size_t>(z)] = true;
        if (previous >= 0) { allowed_[static_cast<std::size_t>(previous)][static_cast<std::size_t>(z)] = allowed_[static_cast<std::size_t>(z)][static_cast<std::size_t>(previous)] = true; }
        previous = z;
      }
      if (previous >= 0 && first >= 0) { entry_[static_cast<std::size_t>(first)] = true; entry_[static_cast<std::size_t>(previous)] = true; any_paths_ = true; }
    }
  }

  const Config& config() const { return config_; }

  void reset() {
    for (Tr& t : tracks_) t = Tr{};
    for (ZoneState& z : zone_state_) z = ZoneState{};
    for (LineState& l : line_state_) l = LineState{};
    for (bool& b : leave_seen_) b = false;
    masked_ = false;
    started_ = false;
    last_target_ms_ = 0;
    dropped_events_ = 0;
    smoother_.set_window(config_.count_window);
  }

  // Switches a zone on or off (an exclusion zone for a window that is open, a detection zone for a gate). False when no enabled zone has the name.
  bool set_zone_active(const std::string& name, bool on) {
    const int z = zone_index(name);
    if (z < 0) return false;
    runtime_active_[static_cast<std::size_t>(z)] = on;
    return true;
  }
  // Forgets what a latched zone holds. False when the name is not a zone.
  bool release_latch(const std::string& name) {
    const int z = zone_index(name);
    if (z < 0) return false;
    pending_release_[static_cast<std::size_t>(z)] = true;
    return true;
  }
  int zone_index(const std::string& name) const {
    for (std::size_t z = 0; z < kMaxZones; ++z) if (config_.zones[z].enabled && config_.zones[z].name == name) return static_cast<int>(z);
    return -1;
  }
  bool zone_is_active(std::size_t z) const { return z < kMaxZones && runtime_active_[z]; }

  // One frame. The events are written to `out` (at most `capacity`; the rest are counted in dropped_events()). Returns how many.
  std::size_t update(const Track* tracks, std::size_t count, const Context& ctx, Event* out, std::size_t capacity) {
    Sink sink{out, capacity, 0, 0};
    const std::uint64_t now = ctx.now_ms;
    if (!started_) { started_ = true; last_target_ms_ = now; }
    if (!config_.enabled) return 0;
    const int profile = ctx.night && config_.night_enabled ? kProfileNight : kProfileDay;
    bool on[kMaxZones];
    for (std::size_t z = 0; z < kMaxZones; ++z) {
      const Zone& zone = config_.zones[z];
      on[z] = zone.enabled && runtime_active_[z] && (!config_.night_enabled || (zone.profiles & profile) != 0) && zone.width_mm > 0 && zone.height_mm > 0;
      zone_state_[z].on = on[z];
    }

    // 1. which targets exist for the rules: inside the range, and not in an exclusion zone that is on
    const Track* kept[kMaximumTracks];
    std::size_t kept_count = 0;
    for (std::size_t i = 0; i < count && kept_count < kMaximumTracks; ++i) {
      const Track& t = tracks[i];
      if (t.y_mm < config_.range_min_mm || t.y_mm > config_.range_max_mm) continue;
      bool excluded = false;
      for (std::size_t z = 0; z < kMaxZones && !excluded; ++z) if (on[z] && config_.zones[z].exclude && geometry::inside(config_.zones[z], t.x_mm, t.y_mm)) excluded = true;
      if (!excluded) kept[kept_count++] = &t;
    }
    smoother_.push(kept_count);
    raw_count_ = kept_count;

    // 2. masking
    const bool watching = config_.masking_minutes > 0 && (ctx.armed || !config_.masking_armed_only);
    if (!watching) {
      last_target_ms_ = now;
      if (masked_) { masked_ = false; sink.add(Event{Event::Type::kMaskingEnd, -1, 0, 0, 0, 0, Movement::kStill, now}); }
    } else if (kept_count > 0) {
      last_target_ms_ = now;
      if (masked_) { masked_ = false; sink.add(Event{Event::Type::kMaskingEnd, -1, 0, 0, 0, 0, Movement::kStill, now}); }
    } else if (!masked_ && now - last_target_ms_ >= static_cast<std::uint64_t>(config_.masking_minutes) * 60000ULL) {
      masked_ = true;
      sink.add(Event{Event::Type::kMasking, -1, 0, 0, config_.masking_minutes, 0, Movement::kStill, now});
    }

    // 3. every target
    for (std::size_t i = 0; i < kept_count; ++i) follow(*kept[i], on, now, sink);

    // 4. targets that are gone
    for (Tr& t : tracks_) {
      if (!t.used || now - t.last_ms <= kLostMs) continue;
      for (std::size_t z = 0; z < kMaxZones; ++z) {
        if (!(t.confirmed & (1u << z))) continue;
        const Zone& zone = config_.zones[z];
        if (zone.latch && on[z] && geometry::edge_distance(zone, t.x, t.y) > kLatchEdgeMm) { zone_state_[z].latched = true; latched_at_[z] = now; leave_seen_[z] = false; }
      }
      t = Tr{};
    }

    // 5. what each zone is now
    for (std::size_t z = 0; z < kMaxZones; ++z) {
      ZoneState& state = zone_state_[z];
      if (config_.zones[z].exclude || !config_.zones[z].enabled) continue;
      unsigned occupants = 0;
      if (on[z]) for (const Tr& t : tracks_) if (t.used && (t.confirmed & (1u << z))) ++occupants;
      state.occupants = occupants;
      if (!on[z]) state.latched = false;
      if (pending_release_[z]) { state.latched = false; pending_release_[z] = false; }
      if (state.latched && occupants == 0 && (leave_seen_[z] || now - latched_at_[z] >= kLatchMaxMs)) state.latched = false;
      const bool occupied = on[z] && (occupants > 0 || state.latched);
      if (occupied && !state.occupied) {
        state.occupied = true;
        state.since_ms = now;
        ++state.entries;
        sink.add(Event{Event::Type::kZoneOccupied, static_cast<int>(z), 0, 0, static_cast<int>(occupants), 0, Movement::kStill, now});
      } else if (!occupied && state.occupied) {
        state.occupied = false;
        state.last_dwell_s = static_cast<std::uint32_t>((now - state.since_ms) / 1000);
        sink.add(Event{Event::Type::kZoneClear, static_cast<int>(z), 0, 0, static_cast<int>(state.last_dwell_s), 0, Movement::kStill, now});
      }
    }
    dropped_events_ += sink.dropped;
    return sink.written;
  }

  // ---- what the panel and the metrics read ----------------------------------------------------------------------------------------------
  const ZoneState& zone(std::size_t z) const { return zone_state_[z < kMaxZones ? z : 0]; }
  const LineState& line(std::size_t l) const { return line_state_[l < kMaxLines ? l : 0]; }
  std::size_t smoothed_count() const { return smoother_.value(); }
  std::size_t raw_count() const { return raw_count_; }
  bool masked() const { return masked_; }
  std::uint32_t dropped_events() const { return dropped_events_; }
  // The class of one target, still by default.
  Movement movement_of(std::uint8_t sensor, std::uint8_t id) const {
    for (const Tr& t : tracks_) if (t.used && t.sensor == sensor && t.id == id) return t.movement;
    return Movement::kStill;
  }
  std::size_t count_of(Movement m) const {
    std::size_t n = 0;
    for (const Tr& t : tracks_) if (t.used && t.movement == m) ++n;
    return n;
  }

 private:
  struct Sink {
    Event* out;
    std::size_t capacity;
    std::size_t written;
    std::uint32_t dropped;
    void add(const Event& e) { if (written < capacity) out[written++] = e; else ++dropped; }
  };

  struct Tr {
    bool used = false;
    std::uint8_t sensor = 0, id = 0;
    std::uint64_t last_ms = 0;
    int x = 0, y = 0;
    int speed = 0;                          // smoothed absolute speed, mm/s
    Movement movement = Movement::kStill;
    unsigned confirmed = 0;                 // bit z: inside zone z (debounced)
    std::uint8_t in_frames[kMaxZones]{}, out_frames[kMaxZones]{};
    std::uint64_t entered_ms[kMaxZones]{};
    bool loitered[kMaxZones]{};
    int side[kMaxLines]{};                  // -1, 0 (unknown) or +1: the side of the line it was last clearly on
    int prev_x[kMaxLines]{}, prev_y[kMaxLines]{};   // where it was in the frame before, for the step that may cross a line
    bool has_prev = false;
    bool stepped_over[kMaxLines]{};         // a step since it was last clearly on `side` went over the line itself (and not round its end)
    int last_zone = -1;
    std::uint64_t last_zone_ms = 0;
  };

  Tr* slot_of(const Track& t, std::uint64_t now, bool& created) {
    created = false;
    Tr* free_slot = nullptr;
    Tr* oldest = nullptr;
    for (Tr& s : tracks_) {
      if (s.used && s.sensor == t.sensor_id && s.id == t.track_id) return &s;
      if (!s.used) { if (free_slot == nullptr) free_slot = &s; }
      else if (oldest == nullptr || s.last_ms < oldest->last_ms) oldest = &s;
    }
    Tr* chosen = free_slot != nullptr ? free_slot : oldest;
    if (chosen == nullptr) return nullptr;
    *chosen = Tr{};
    chosen->used = true;
    chosen->sensor = t.sensor_id;
    chosen->id = t.track_id;
    chosen->last_ms = now;
    created = true;
    return chosen;
  }

  void follow(const Track& track, const bool* on, std::uint64_t now, Sink& sink) {
    bool fresh = false;
    Tr* slot = slot_of(track, now, fresh);
    if (slot == nullptr) return;
    Tr& tr = *slot;
    tr.last_ms = now;
    tr.x = track.x_mm;
    tr.y = track.y_mm;
    const int speed = std::abs(static_cast<int>(track.speed_mm_s));
    tr.speed = fresh ? speed : (tr.speed * 2 + speed) / 3;
    tr.movement = classify(tr.speed, tr.movement);

    // zones
    for (std::size_t z = 0; z < kMaxZones; ++z) {
      const Zone& zone = config_.zones[z];
      if (!zone.enabled || zone.exclude) continue;
      const unsigned bit = 1u << z;
      if (!on[z]) { tr.confirmed &= ~bit; tr.in_frames[z] = tr.out_frames[z] = 0; continue; }
      const bool inside = geometry::inside(zone, track.x_mm, track.y_mm);
      if (inside) { if (tr.in_frames[z] < 255) ++tr.in_frames[z]; tr.out_frames[z] = 0; }
      else { if (tr.out_frames[z] < 255) ++tr.out_frames[z]; tr.in_frames[z] = 0; }
      const int need = config_.debounce_frames < 1 ? 1 : config_.debounce_frames;
      if (!(tr.confirmed & bit) && tr.in_frames[z] >= need) {
        tr.confirmed |= bit;
        tr.entered_ms[z] = now;
        tr.loitered[z] = false;
        entering(tr, z, now, sink);
      } else if ((tr.confirmed & bit) && tr.out_frames[z] >= need) {
        tr.confirmed &= ~bit;
        leave_seen_[z] = true;   // seen leaving, alive: a latched zone may let go
      }
      if (tr.confirmed & bit) {
        tr.last_zone_ms = now;
        if (zone.loiter_s > 0 && !tr.loitered[z] && now - tr.entered_ms[z] >= static_cast<std::uint64_t>(zone.loiter_s) * 1000ULL) {
          tr.loitered[z] = true;
          sink.add(Event{Event::Type::kLoiter, static_cast<int>(z), tr.sensor, tr.id, zone.loiter_s, 0, tr.movement, now});
        }
      }
    }

    // lines: a crossing is counted when the target is clearly on one side, was clearly on the other before, and a step in between went over the line itself
    for (std::size_t l = 0; l < kMaxLines; ++l) {
      const Line& line = config_.lines[l];
      if (!line.enabled) continue;
      const double dx = line.bx_mm - line.ax_mm, dy = line.by_mm - line.ay_mm;
      const double length = std::sqrt(dx * dx + dy * dy);
      if (length < 1.0) continue;
      if (tr.has_prev && geometry::segments_cross(tr.prev_x[l], tr.prev_y[l], track.x_mm, track.y_mm, line.ax_mm, line.ay_mm, line.bx_mm, line.by_mm)) tr.stepped_over[l] = true;
      tr.prev_x[l] = track.x_mm;
      tr.prev_y[l] = track.y_mm;
      const double distance = (dx * (track.y_mm - line.ay_mm) - dy * (track.x_mm - line.ax_mm)) / length;   // positive: left of a->b
      if (std::fabs(distance) < kLineHysteresisMm) continue;
      const int side = distance > 0 ? 1 : -1;
      if (tr.side[l] == 0 || side == tr.side[l]) { tr.side[l] = side; tr.stepped_over[l] = false; continue; }
      const bool crossed = tr.stepped_over[l];
      const int before = tr.side[l];
      tr.side[l] = side;
      tr.stepped_over[l] = false;
      if (!crossed) continue;   // it went round the end of the line
      const int direction = before > 0 ? 1 : -1;   // from the left of a->b to its right: in
      LineState& state = line_state_[l];
      if (direction > 0) ++state.ins; else ++state.outs;
      sink.add(Event{Event::Type::kTripwire, static_cast<int>(l), tr.sensor, tr.id, direction, static_cast<int>(direction > 0 ? state.ins : state.outs), tr.movement, now});
    }
    tr.has_prev = true;
  }

  // A target has just been confirmed inside zone z: is the way it got there one of the paths?
  void entering(Tr& tr, std::size_t z, std::uint64_t now, Sink& sink) {
    if (any_paths_ && managed_[z]) {
      const int last = tr.last_zone;
      bool violation = false;
      int from = -1;
      if (last >= 0 && last != static_cast<int>(z) && now - tr.last_zone_ms <= kPathWindowMs && managed_[static_cast<std::size_t>(last)]) { from = last; violation = !allowed_[static_cast<std::size_t>(last)][z]; }
      else if (last < 0 && config_.strict_entry && !entry_[z]) violation = true;
      if (violation) sink.add(Event{Event::Type::kPathViolation, static_cast<int>(z), tr.sensor, tr.id, from, 0, tr.movement, now});
    }
    tr.last_zone = static_cast<int>(z);
    tr.last_zone_ms = now;
  }

  Config config_;
  std::array<Tr, kMaximumTracks + 1> tracks_{};
  std::array<ZoneState, kMaxZones> zone_state_{};
  std::array<LineState, kMaxLines> line_state_{};
  bool runtime_active_[kMaxZones]{};
  bool pending_release_[kMaxZones]{};
  bool leave_seen_[kMaxZones]{};
  std::uint64_t latched_at_[kMaxZones]{};
  bool managed_[kMaxZones]{}, entry_[kMaxZones]{};
  bool allowed_[kMaxZones][kMaxZones]{};
  bool any_paths_ = false;
  bool masked_ = false;
  bool started_ = false;
  std::uint64_t last_target_ms_ = 0;
  std::uint32_t dropped_events_ = 0;
  std::size_t raw_count_ = 0;
  CountSmoother smoother_;
};

}  // namespace armor::perimeter
