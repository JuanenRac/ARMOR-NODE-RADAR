// ARMOR-NODE-RADAR - host tests for the way the reader task waits for its serial lines (main/core/uart_wake.hpp): what each event asks for, how long the task may sleep, and a
// made-up sensor line played against the old polling and the new waiting, with a stall of the task in the middle (a write to the flash stops it for a while).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>

#include "../main/core/uart_wake.hpp"

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

using namespace armor::uartwake;

static void test_settings() {
  CHECK(idle_ms_is_valid(0) && idle_ms_is_valid(10) && idle_ms_is_valid(100) && idle_ms_is_valid(500));
  CHECK(!idle_ms_is_valid(-1) && !idle_ms_is_valid(1) && !idle_ms_is_valid(9) && !idle_ms_is_valid(501));
  CHECK(!events_enabled(0) && events_enabled(10) && events_enabled(kIdleMsDefault));
  CHECK(kIdleMsDefault == 100 && kPollMs == 10);
}

static void test_verdicts() {
  CHECK(judge(Event::kData).read && !judge(Event::kData).flush && !judge(Event::kData).overrun);
  for (const Event overflow : {Event::kFifoOverflow, Event::kBufferFull}) {
    const Verdict v = judge(overflow);
    CHECK(v.read && v.flush && v.overrun && !v.line_error);   // the bytes lost are counted and what is queued is cut frames
  }
  for (const Event noise : {Event::kBreak, Event::kParityError, Event::kFrameError}) {
    const Verdict v = judge(noise);
    CHECK(v.read && v.line_error && !v.flush && !v.overrun);
  }
  const Verdict other = judge(Event::kOther);
  CHECK(!other.read && !other.flush && !other.overrun && !other.line_error);
}

static void test_sleep() {
  CHECK(sleep_ms(1000, 0, 0, 0) == kPollMs);                // polling mode: the old fixed wait, whatever the deadlines
  CHECK(sleep_ms(0, 0, 0, 100) == 100);                     // nothing due soon: the idle time
  CHECK(sleep_ms(0, 0, 0, 500) == 500);
  CHECK(sleep_ms(4950, 0, 0, 100) == 50);                   // the frame-rate window is due in 50 ms: do not oversleep it
  CHECK(sleep_ms(9990, 5000, 0, 100) == 10);                // the statistics are due in 10 ms
  CHECK(sleep_ms(6000, 0, 0, 100) == 1);                    // late: one millisecond, never zero (no spinning) and never a long sleep
  CHECK(sleep_ms(12000, 5000, 0, 100) == 1);
  for (std::uint64_t now = 0; now < 30000; now += 37)
    for (const int idle : {10, 100, 500}) {
      const unsigned wait = sleep_ms(now, now / 5000 * 5000, now / 10000 * 10000, idle);
      CHECK(wait >= 1 && wait <= static_cast<unsigned>(idle));
    }
}

static void test_counters() {
  Pacer pacer;
  pacer.turn(0);
  pacer.turn(0b001);
  pacer.turn(0b101);
  CHECK(pacer.counters().polls == 3 && pacer.counters().idle_wakeups == 1);
  CHECK(pacer.counters().line[0].wakeups == 2 && pacer.counters().line[1].wakeups == 0 && pacer.counters().line[2].wakeups == 1);
  CHECK(pacer.note(1, Event::kBufferFull).flush);
  CHECK(pacer.note(1, Event::kFifoOverflow).overrun);
  CHECK(pacer.counters().line[1].overruns == 2 && pacer.counters().line[1].flushes == 2);
  pacer.note(2, Event::kParityError);
  CHECK(pacer.counters().line[2].line_errors == 1 && pacer.counters().line[0].line_errors == 0);
  pacer.note(7, Event::kBufferFull);   // a line that does not exist changes nothing
  CHECK(pacer.counters().line[0].overruns == 0);
}

// A sensor that sends a frame of `frame_bytes` every `period_ms`; the driver's ring buffer holds `ring` bytes. The task either looks every 10 ms (idle_ms 0) or sleeps until an event,
// with `idle_ms` as the longest sleep, and is stopped for `stall_ms` at `stall_at` (a write to the flash that stops the cache). Nothing here is the chip: it is the loop of the reader
// task with a made-up clock. The worst latency is the longest a byte waited between arriving and being read.
struct Outcome {
  unsigned turns = 0, frames_read = 0, overflows = 0;
  std::uint64_t worst_latency_ms = 0;
};

struct Line {
  unsigned frame_bytes, ring;
  double buffered = 0;
  std::uint64_t oldest_ms = 0;
  unsigned overflows = 0;
  void arrive(std::uint64_t at) {
    if (buffered == 0) oldest_ms = at;
    buffered += frame_bytes;
    if (buffered > ring) { buffered = ring; ++overflows; }
  }
};

static Outcome play(int idle_ms, unsigned period_ms, unsigned frame_bytes, unsigned ring, std::uint64_t seconds, std::uint64_t stall_at, std::uint64_t stall_ms) {
  Outcome out;
  Line line{frame_bytes, ring};
  std::uint64_t now = 0, last_window = 0, last_stats = 0, next_frame = period_ms;
  bool stalled = false;
  Pacer pacer;
  while (now < seconds * 1000) {
    std::uint64_t wake = now + sleep_ms(now, last_window, last_stats, idle_ms);   // the task decides how long to sleep ...
    unsigned woken = 0;
    if (events_enabled(idle_ms) && next_frame < wake) { wake = next_frame; woken = 1; }   // ... and an event wakes it earlier
    while (next_frame <= wake) { line.arrive(next_frame); next_frame += period_ms; }
    now = wake;
    if (!stalled && stall_ms != 0 && now >= stall_at) {
      stalled = true;
      now += stall_ms;
      while (next_frame <= now) { line.arrive(next_frame); next_frame += period_ms; }
    }
    pacer.turn(woken);
    ++out.turns;
    if (line.buffered > 0) {   // the task drains everything that is there
      out.frames_read += static_cast<unsigned>(line.buffered / frame_bytes);
      if (now - line.oldest_ms > out.worst_latency_ms) out.worst_latency_ms = now - line.oldest_ms;
      line.buffered = 0;
    }
    if (now - last_window >= kWindowEveryMs) last_window = now;
    if (now - last_stats >= kStatsEveryMs) last_stats = now;
  }
  out.overflows = line.overflows;
  return out;
}

static void test_a_sensor_line_polled_and_waited_for() {
  // an LD2450: 30-byte frames, ten a second, a 2048-byte ring; ten seconds, with the task stopped for 300 ms at the fifth second
  const Outcome polled = play(0, 100, 30, 2048, 10, 5000, 300);
  const Outcome waited = play(100, 100, 30, 2048, 10, 5000, 300);
  CHECK(polled.turns >= 900);                                       // looking every 10 ms: a hundred turns a second
  CHECK(waited.turns < polled.turns / 3);                           // sleeping until something arrives: about the frames and the idle turns
  CHECK(polled.frames_read >= 95 && waited.frames_read >= 95);      // every frame of the ten seconds is read by both
  CHECK(polled.overflows == 0 && waited.overflows == 0);
  CHECK(waited.worst_latency_ms <= 300 && polled.worst_latency_ms <= 300);   // the longest is the stall, and nothing else adds to it
  // without the stall the bytes are read at once with the wake-up, and within the polling interval with the polling
  const Outcome quick = play(100, 100, 30, 2048, 10, 0, 0);
  const Outcome looked = play(0, 100, 30, 2048, 10, 0, 0);
  CHECK(quick.worst_latency_ms == 0);
  CHECK(looked.worst_latency_ms <= kPollMs);
}

static void test_a_long_stall_overflows_the_ring_and_is_counted() {
  // frames of 128 bytes every 5 ms across a 600 ms stall overflow a 2048-byte ring, which is what the counter is for; the same line without the stall loses nothing
  const Outcome stalled = play(100, 5, 128, 2048, 3, 1000, 600);
  CHECK(stalled.overflows > 0);
  const Outcome calm = play(100, 5, 128, 2048, 3, 0, 0);
  CHECK(calm.overflows == 0);
}

int main() {
  test_settings();
  test_verdicts();
  test_sleep();
  test_counters();
  test_a_sensor_line_polled_and_waited_for();
  test_a_long_stall_overflows_the_ring_and_is_counted();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
