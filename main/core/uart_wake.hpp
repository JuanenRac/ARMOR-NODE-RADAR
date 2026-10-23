// ARMOR-NODE-RADAR - what the task that reads the sensors does when its serial lines wake it, instead of looking at them every few milliseconds. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The serial driver can tell a task that bytes arrived (an event on a queue). The reader task sleeps on the three queues together and wakes when a sensor has sent something, or when
// the idle time has passed (the periodic work of the task: the presence devices, the frame-rate window, the statistics). This file holds the decisions that do not need a chip:
// what each kind of event asks for, how long the task may sleep, and the counters that show how the lines behave. The calls to the driver are in main/radar_manager.cpp.
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace armor::uartwake {

// The setting `system.reader_idle_ms`: 0 keeps the polling every kPollMs of the first versions; otherwise the most the task sleeps when no sensor says anything.
constexpr int kIdleMsMin = 10;
constexpr int kIdleMsMax = 500;
constexpr int kIdleMsDefault = 100;
constexpr unsigned kPollMs = 10;

inline bool idle_ms_is_valid(int value) { return value == 0 || (value >= kIdleMsMin && value <= kIdleMsMax); }
inline bool events_enabled(int idle_ms) { return idle_ms != 0; }

// What the driver can report about a line (a subset of its own event types, so this file knows nothing of the chip).
enum class Event { kData, kFifoOverflow, kBufferFull, kBreak, kParityError, kFrameError, kOther };

struct Verdict {
  bool read = false;       // bytes are waiting: drain the line now
  bool flush = false;      // the bytes are no use any more (the buffer overflowed): drop what is queued so the next frame starts clean
  bool overrun = false;    // count it as bytes the sensor sent and the node lost
  bool line_error = false; // count it as a fault of the line (noise, a wrong speed)
};

inline Verdict judge(Event event) {
  Verdict v;
  switch (event) {
    case Event::kData: v.read = true; break;
    case Event::kFifoOverflow:
    case Event::kBufferFull: v.read = true; v.flush = true; v.overrun = true; break;   // the framers resynchronise on the next header; half a frame is worse than none
    case Event::kBreak:
    case Event::kParityError:
    case Event::kFrameError: v.read = true; v.line_error = true; break;
    case Event::kOther: break;
  }
  return v;
}

struct LineCounters {
  std::uint32_t wakeups = 0;       // times the task woke because of this line
  std::uint32_t overruns = 0;      // buffer overflows
  std::uint32_t line_errors = 0;   // breaks, parity and framing errors
  std::uint32_t flushes = 0;
};

struct Counters {
  std::array<LineCounters, 3> line{};
  std::uint32_t idle_wakeups = 0;  // times the task woke with nothing from any line
  std::uint32_t polls = 0;         // turns of the loop in all
};

// The deadlines of the periodic work, in milliseconds on the node's clock.
constexpr std::uint64_t kWindowEveryMs = 5000;
constexpr std::uint64_t kStatsEveryMs = 10000;

// How long the task may sleep from `now`: never past the idle time, never past the next window or statistics deadline, never less than one millisecond (a late deadline must not make
// the task spin). In polling mode (idle_ms 0) the sleep is the old fixed one.
inline unsigned sleep_ms(std::uint64_t now, std::uint64_t last_window, std::uint64_t last_stats, int idle_ms) {
  if (!events_enabled(idle_ms)) return kPollMs;
  std::uint64_t wait = static_cast<std::uint64_t>(idle_ms);
  const std::uint64_t window_due = last_window + kWindowEveryMs, stats_due = last_stats + kStatsEveryMs;
  wait = std::min<std::uint64_t>(wait, window_due > now ? window_due - now : 1);
  wait = std::min<std::uint64_t>(wait, stats_due > now ? stats_due - now : 1);
  return static_cast<unsigned>(std::max<std::uint64_t>(wait, 1));
}

// One turn of the loop, as the counters see it: `woken` is the set of lines that raised an event (bit i for line i) and `events` the events taken from them.
class Pacer {
 public:
  void turn(unsigned woken_lines) {
    ++counters_.polls;
    if (woken_lines == 0) ++counters_.idle_wakeups;
    for (std::size_t i = 0; i < 3; ++i) if (woken_lines & (1u << i)) ++counters_.line[i].wakeups;
  }
  // The verdict of one event of one line; the counters keep what it means. The caller does what the verdict says.
  Verdict note(std::size_t line, Event event) {
    const Verdict v = judge(event);
    if (line < 3) {
      if (v.overrun) ++counters_.line[line].overruns;
      if (v.line_error) ++counters_.line[line].line_errors;
      if (v.flush) ++counters_.line[line].flushes;
    }
    return v;
  }
  const Counters& counters() const { return counters_; }

 private:
  Counters counters_;
};

}  // namespace armor::uartwake
