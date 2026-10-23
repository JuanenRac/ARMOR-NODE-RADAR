// ARMOR-NODE-RADAR - learns where the static reflectors of a yard are, and suggests exclusion zones for them. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// A metal gate, a ventilation grille or a ceiling fan makes the radar report a target that never moves. The operator starts a learning run with the area empty (a few minutes); the learner counts,
// in a map of 80 x 80 cells of 10 cm (8 m by 8 m, x from -4 m to +4 m, y from 0 to 8 m), the frames in which a still target was in each cell. The cells that held one in a good part of the run are
// joined into groups, and each group becomes a rectangle suggested as an exclusion zone. Nothing is applied by itself: a person who looks at the suggestion adds the zone.
// A person who stands still during the run is learned as a reflector too; the panel says to leave the area empty.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "../radar_tracks.hpp"

namespace armor::reflectors {

constexpr int kCols = 80, kRows = 80, kCellMm = 100;
constexpr int kOriginXMm = -4000, kOriginYMm = 0;
constexpr std::size_t kMaxSuggestions = 3;

struct Suggestion {
  int x_mm = 0, y_mm = 0, width_mm = 0, height_mm = 0;   // angle 0
  unsigned frames = 0;                                    // the most frames any cell of the group held a still target
};

class Learner {
 public:
  static constexpr int kStillMmS = 40;

  Learner() : hits_(static_cast<std::size_t>(kCols) * kRows, 0) {}

  void start(std::uint64_t now_ms, std::uint32_t duration_ms) {
    for (std::uint8_t& h : hits_) h = 0;
    frames_ = 0;
    started_ms_ = now_ms;
    duration_ms_ = duration_ms == 0 ? 1 : duration_ms;
    active_ = true;
  }
  void stop() { active_ = false; }
  bool active() const { return active_; }
  unsigned frames() const { return frames_; }
  unsigned progress_percent(std::uint64_t now_ms) const {
    if (!active_) return frames_ > 0 ? 100 : 0;
    const std::uint64_t elapsed = now_ms - started_ms_;
    return elapsed >= duration_ms_ ? 100 : static_cast<unsigned>(elapsed * 100 / duration_ms_);
  }

  // One frame of the node's targets (shared plane). The run ends by itself when its time is up.
  void feed(const Track* tracks, std::size_t count, std::uint64_t now_ms) {
    if (!active_) return;
    if (now_ms - started_ms_ >= duration_ms_) { active_ = false; return; }
    ++frames_;
    for (std::size_t i = 0; i < count; ++i) {
      if (std::abs(static_cast<int>(tracks[i].speed_mm_s)) > kStillMmS) continue;
      if (tracks[i].x_mm < kOriginXMm || tracks[i].y_mm < kOriginYMm) continue;
      const int cx = (tracks[i].x_mm - kOriginXMm) / kCellMm, cy = (tracks[i].y_mm - kOriginYMm) / kCellMm;
      if (cx >= kCols || cy >= kRows) continue;
      std::uint8_t& cell = hits_[static_cast<std::size_t>(cy) * kCols + cx];
      if (cell < 255) ++cell;
    }
  }

  // The groups of cells that held a still target in at least `percent` of the frames (and at least `min_frames` of them), as rectangles, the strongest first.
  std::size_t suggest(Suggestion* out, std::size_t capacity, unsigned percent = 25, unsigned min_frames = 20) const {
    if (frames_ == 0) return 0;
    unsigned threshold = static_cast<unsigned>((static_cast<unsigned long long>(frames_) * percent + 99) / 100);
    if (threshold < min_frames) threshold = min_frames;
    if (threshold > 255) threshold = 255;   // a cell counts up to 255
    std::vector<std::uint8_t> seen(hits_.size(), 0);
    std::vector<int> stack;
    Suggestion all[16];
    std::size_t found = 0;
    for (int y = 0; y < kRows && found < 16; ++y) {
      for (int x = 0; x < kCols && found < 16; ++x) {
        const std::size_t at = static_cast<std::size_t>(y) * kCols + x;
        if (seen[at] || hits_[at] < threshold) continue;
        stack.clear();
        stack.push_back(static_cast<int>(at));
        seen[at] = 1;
        int min_x = x, max_x = x, min_y = y, max_y = y;
        unsigned strongest = 0;
        while (!stack.empty()) {
          const int cell = stack.back();
          stack.pop_back();
          const int cx = cell % kCols, cy = cell / kCols;
          if (cx < min_x) min_x = cx;
          if (cx > max_x) max_x = cx;
          if (cy < min_y) min_y = cy;
          if (cy > max_y) max_y = cy;
          if (hits_[static_cast<std::size_t>(cell)] > strongest) strongest = hits_[static_cast<std::size_t>(cell)];
          for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
              const int nx = cx + dx, ny = cy + dy;
              if (nx < 0 || ny < 0 || nx >= kCols || ny >= kRows) continue;
              const std::size_t n = static_cast<std::size_t>(ny) * kCols + nx;
              if (seen[n] || hits_[n] < threshold) continue;
              seen[n] = 1;
              stack.push_back(static_cast<int>(n));
            }
          }
        }
        Suggestion s;
        s.x_mm = kOriginXMm + (min_x - 1) * kCellMm;   // one cell of margin around the group
        s.y_mm = kOriginYMm + (min_y - 1) * kCellMm;
        s.width_mm = (max_x - min_x + 3) * kCellMm;
        s.height_mm = (max_y - min_y + 3) * kCellMm;
        s.frames = strongest;
        all[found++] = s;
      }
    }
    for (std::size_t i = 1; i < found; ++i) { const Suggestion key = all[i]; std::size_t j = i; while (j > 0 && all[j - 1].frames < key.frames) { all[j] = all[j - 1]; --j; } all[j] = key; }
    std::size_t written = 0;
    for (std::size_t i = 0; i < found && written < capacity && written < kMaxSuggestions; ++i) out[written++] = all[i];
    return written;
  }

 private:
  std::vector<std::uint8_t> hits_;
  unsigned frames_ = 0;
  std::uint64_t started_ms_ = 0;
  std::uint32_t duration_ms_ = 180000;
  bool active_ = false;
};

}  // namespace armor::reflectors
