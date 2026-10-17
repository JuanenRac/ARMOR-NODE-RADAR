// ARMOR-RADAR - unifying three radars' own local coordinates onto one shared plane.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Each LD2450 reports X/Y in millimetres from its own position, facing its own direction. With three of them aimed in different
// directions (say 0, 120 and 240 degrees for all-round coverage, or all forward for a long corridor), a target's raw X/Y only means
// something once it is rotated by the radar's own heading (yaw) and moved to the radar's own position (offset) on the node's shared
// plane. A radar aimed slightly down (pitch) also reports a straight-line range a little longer than the ground distance; that is
// corrected first, before the rotation.
//
// A radar left at all zeros (the default) is returned exactly as it arrived: nothing here changes what a node with no calibration set
// has always published.
#pragma once
#include <cmath>
#include <cstdint>

#include "../radar_tracks.hpp"
#include "node_config.hpp"

namespace armor::calibration {

namespace detail {
inline std::int16_t clamp_mm(double value) {
  if (value > 32767.0) return 32767;
  if (value < -32768.0) return -32768;
  return static_cast<std::int16_t>(std::lround(value));
}
}  // namespace detail

inline Track apply(const Track& raw, const config::RadarLine& radar) {
  Track out = raw;
  if (radar.offset_x_mm == 0 && radar.offset_y_mm == 0 && radar.yaw_deg == 0 && radar.pitch_deg == 0) return out;
  constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
  const double pitch = static_cast<double>(radar.pitch_deg) * kDegToRad;
  const double yaw = static_cast<double>(radar.yaw_deg) * kDegToRad;
  const double x = static_cast<double>(raw.x_mm);
  const double y_level = static_cast<double>(raw.y_mm) * std::cos(pitch);   // foreshortened by the downward tilt, if any
  const double cos_yaw = std::cos(yaw), sin_yaw = std::sin(yaw);
  out.x_mm = detail::clamp_mm(x * cos_yaw - y_level * sin_yaw + static_cast<double>(radar.offset_x_mm));
  out.y_mm = detail::clamp_mm(x * sin_yaw + y_level * cos_yaw + static_cast<double>(radar.offset_y_mm));
  return out;
}

// Applies apply() to every track, using each one's own sensor_id (1..3) to pick its radar's calibration. A track with an out-of-range
// sensor_id is left untouched (track_is_valid() already keeps that from happening in practice).
inline void apply_all(Track* tracks, std::size_t count, const std::array<config::RadarLine, kRadarCount>& radars) {
  for (std::size_t i = 0; i < count; ++i) {
    const std::uint8_t sensor = tracks[i].sensor_id;
    if (sensor >= 1 && sensor <= kRadarCount) tracks[i] = apply(tracks[i], radars[sensor - 1]);
  }
}

// Two tracks from different sensors that land within `threshold_mm` of each other on the shared plane are almost certainly the same
// person seen by two radars at once, not two people standing in exactly the same spot - merged into their midpoint (speed averaged
// too) rather than published twice. A threshold of 0 turns this off: every track is published as its radar reported it.
// `out` must be large enough for `count` tracks (it can only shrink the list); returns how many are left after merging.
inline std::size_t merge_overlap(Track* tracks, std::size_t count, int threshold_mm) {
  if (threshold_mm <= 0) return count;
  bool merged[kMaximumTracks] = {};
  std::size_t kept = 0;
  for (std::size_t i = 0; i < count; ++i) {
    if (merged[i]) continue;
    // the sum is kept in doubles (three radars at 12 m would overflow sixteen bits), and a group takes at most one track of each radar: two people seen by the second radar near one
    // seen by the first are two people, not one
    double sum_x = tracks[i].x_mm, sum_y = tracks[i].y_mm, sum_speed = tracks[i].speed_mm_s;
    unsigned radars_in_group = 1u << (tracks[i].sensor_id & 7);
    int votes = 1;
    for (std::size_t j = i + 1; j < count; ++j) {
      if (merged[j] || tracks[j].sensor_id == tracks[i].sensor_id || (radars_in_group & (1u << (tracks[j].sensor_id & 7))) != 0) continue;   // only a different radar can be "the same person"
      const double dx = static_cast<double>(tracks[j].x_mm) - static_cast<double>(tracks[i].x_mm);
      const double dy = static_cast<double>(tracks[j].y_mm) - static_cast<double>(tracks[i].y_mm);
      if (std::sqrt(dx * dx + dy * dy) > static_cast<double>(threshold_mm)) continue;
      sum_x += tracks[j].x_mm; sum_y += tracks[j].y_mm; sum_speed += tracks[j].speed_mm_s;
      radars_in_group |= 1u << (tracks[j].sensor_id & 7);
      merged[j] = true;
      ++votes;
    }
    Track average = tracks[i];
    if (votes > 1) {
      average.x_mm = detail::clamp_mm(sum_x / votes);
      average.y_mm = detail::clamp_mm(sum_y / votes);
      average.speed_mm_s = detail::clamp_mm(sum_speed / votes);
    }
    tracks[kept++] = average;
  }
  return kept;
}

}  // namespace armor::calibration
