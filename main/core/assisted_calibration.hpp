// ARMOR-NODE-RADAR - assisted alignment of a radar on the shared plane: the installer stands for a few seconds on two or more marked points whose place on the plane is known, the radar
// reads where it sees them, and the heading (yaw) and the position (offset) of the radar are worked out from that, instead of being guessed. Pure: tested on a computer with a simulated
// radar. Nothing is applied by itself: the result is shown, and a person copies it into the radar's settings (radars[].yaw_deg, offset_x_mm, offset_y_mm).
// The convention is the one of radar_calibration.hpp: shared = R(yaw) * raw + offset, with the yaw counted counter-clockwise.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <cstdint>

#include "../radar_tracks.hpp"

namespace armor::calibration {

// Where the person stood on the shared plane (the marked point), and what the radar read there (the mean of a few seconds), both in millimetres.
struct RefPoint {
  double ref_x = 0, ref_y = 0;
  double raw_x = 0, raw_y = 0;
};

struct Alignment {
  bool ok = false;
  double yaw_deg = 0, offset_x_mm = 0, offset_y_mm = 0;
  double rms_mm = 0;          // how far the points are, on average, from their marks after the alignment
  int yaw_setting = 0;        // the same, rounded to what the settings take (-180 to 180)
  int offset_x_setting = 0, offset_y_setting = 0;   // -5000 to 5000
  bool in_range = false;      // the rounded values fit the settings
};

inline int round_to_int(double value) { return static_cast<int>(std::lround(value)); }

// Least squares for a rotation and a shift (a rigid fit). It needs two points at least, 300 mm apart or more on the plane (two points on top of each other say nothing of the heading), and
// refuses a fit whose points are further than `max_rms_mm` from their marks: a badly placed person or a wrong mark, better asked again than written into the settings.
inline Alignment align(const RefPoint* points, std::size_t count, double max_rms_mm = 250.0) {
  Alignment out;
  if (points == nullptr || count < 2) return out;
  double ref_cx = 0, ref_cy = 0, raw_cx = 0, raw_cy = 0;
  for (std::size_t i = 0; i < count; ++i) { ref_cx += points[i].ref_x; ref_cy += points[i].ref_y; raw_cx += points[i].raw_x; raw_cy += points[i].raw_y; }
  const double n = static_cast<double>(count);
  ref_cx /= n; ref_cy /= n; raw_cx /= n; raw_cy /= n;
  double dot = 0, cross = 0, spread = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const double ax = points[i].raw_x - raw_cx, ay = points[i].raw_y - raw_cy;
    const double bx = points[i].ref_x - ref_cx, by = points[i].ref_y - ref_cy;
    dot += ax * bx + ay * by;
    cross += ax * by - ay * bx;
    spread = spread > std::sqrt(bx * bx + by * by) ? spread : std::sqrt(bx * bx + by * by);
  }
  if (spread < 150.0) return out;   // all the marks within 300 mm of each other
  const double yaw = std::atan2(cross, dot);
  const double c = std::cos(yaw), s = std::sin(yaw);
  out.yaw_deg = yaw * 180.0 / 3.14159265358979323846;
  out.offset_x_mm = ref_cx - (c * raw_cx - s * raw_cy);
  out.offset_y_mm = ref_cy - (s * raw_cx + c * raw_cy);
  double squares = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const double x = c * points[i].raw_x - s * points[i].raw_y + out.offset_x_mm - points[i].ref_x;
    const double y = s * points[i].raw_x + c * points[i].raw_y + out.offset_y_mm - points[i].ref_y;
    squares += x * x + y * y;
  }
  out.rms_mm = std::sqrt(squares / n);
  out.yaw_setting = round_to_int(out.yaw_deg);
  out.offset_x_setting = round_to_int(out.offset_x_mm);
  out.offset_y_setting = round_to_int(out.offset_y_mm);
  out.in_range = std::abs(out.offset_x_setting) <= 5000 && std::abs(out.offset_y_setting) <= 5000;   // the heading from atan2 is always within the 180 degrees the settings take
  out.ok = out.rms_mm <= max_rms_mm;
  return out;
}

// The session at the radar: for each mark the person stands still and the radar's readings are added up; `finish_point` keeps the mean once there are enough of them.
class AlignSession {
 public:
  static constexpr std::size_t kMaxPoints = 6;

  // Starts a mark at (ref_x, ref_y) on the shared plane. False when six marks are kept already or one is still being read.
  bool begin_point(double ref_x, double ref_y) {
    if (count_ >= kMaxPoints || reading_) return false;
    current_ = RefPoint();
    current_.ref_x = ref_x; current_.ref_y = ref_y;
    sum_x_ = 0; sum_y_ = 0; samples_ = 0;
    reading_ = true;
    return true;
  }

  void sample(double raw_x, double raw_y) {
    if (!reading_) return;
    sum_x_ += raw_x; sum_y_ += raw_y; ++samples_;
  }

  // One look at the raw targets (before the radar's own calibration): the reading counts when exactly one target of `sensor_id` is seen. With two or more at once there is no telling which one is
  // the person standing on the mark, so the look is left out. True when it counted.
  bool feed_tracks(const armor::Track* tracks, std::size_t count, std::uint8_t sensor_id) {
    if (!reading_ || tracks == nullptr) return false;
    const armor::Track* only = nullptr;
    for (std::size_t i = 0; i < count; ++i) {
      if (!armor::track_is_valid(tracks[i]) || tracks[i].sensor_id != sensor_id) continue;
      if (only != nullptr) return false;
      only = &tracks[i];
    }
    if (only == nullptr) return false;
    sample(only->x_mm, only->y_mm);
    return true;
  }

  // Keeps the mark when it had `min_samples` readings at least; the person is asked to stay otherwise (the mark stays open).
  bool finish_point(std::size_t min_samples = 20) {
    if (!reading_ || samples_ < min_samples) return false;
    current_.raw_x = sum_x_ / static_cast<double>(samples_);
    current_.raw_y = sum_y_ / static_cast<double>(samples_);
    points_[count_++] = current_;
    reading_ = false;
    return true;
  }

  void reset() { count_ = 0; reading_ = false; samples_ = 0; }
  std::size_t points() const { return count_; }
  bool reading() const { return reading_; }
  std::size_t samples() const { return samples_; }
  Alignment result(double max_rms_mm = 250.0) const { return align(points_, count_, max_rms_mm); }

 private:
  RefPoint points_[kMaxPoints];
  RefPoint current_;
  std::size_t count_ = 0, samples_ = 0;
  double sum_x_ = 0, sum_y_ = 0;
  bool reading_ = false;
};

}  // namespace armor::calibration
