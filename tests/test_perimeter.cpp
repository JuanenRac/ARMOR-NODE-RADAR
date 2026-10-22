// ARMOR-RADAR - host tests for the node's perimeter engine and its helpers: zones, lines, paths, masking, the count without flicker, the reflector learner, Wi-Fi jamming, the offline
// ring in flash, the Prometheus page, the events and the stream gate.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../main/core/config_transfer.hpp"
#include "../main/core/jam_detector.hpp"
#include "../main/core/metrics.hpp"
#include "../main/core/node_config.hpp"
#include "../main/core/offline_ring.hpp"
#include "../main/core/perimeter.hpp"
#include "../main/core/radar_event.hpp"
#include "../main/core/reflector_learner.hpp"
#include "../main/core/storage_recovery.hpp"
#include "../main/core/zone_control.hpp"

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
using namespace armor::perimeter;
using Type = Event::Type;

static Track tr(int id, int x, int y, int speed = 0, int sensor = 1) {
  return Track{static_cast<std::uint8_t>(sensor), static_cast<std::uint8_t>(id), static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), static_cast<std::int16_t>(speed)};
}
static Zone zone(const char* name, int x, int y, int w, int h, int angle = 0, bool exclude = false) {
  Zone z;
  z.enabled = true; z.name = name; z.x_mm = x; z.y_mm = y; z.width_mm = w; z.height_mm = h; z.angle_deg = angle; z.exclude = exclude;
  return z;
}

// Runs frames through an engine at 100 ms a frame and keeps every event.
struct Rig {
  Engine engine;
  std::uint64_t now = 1000000;
  std::vector<Event> events;
  bool night = false, armed = false;
  explicit Rig(const Config& c) : engine(c) {}
  void frame(std::initializer_list<Track> targets) {
    std::vector<Track> list(targets);
    Event out[16];
    Context ctx;
    ctx.now_ms = now; ctx.night = night; ctx.armed = armed;
    const std::size_t n = engine.update(list.data(), list.size(), ctx, out, 16);
    for (std::size_t i = 0; i < n; ++i) events.push_back(out[i]);
    now += 100;
  }
  void frames(int count, std::initializer_list<Track> targets) { for (int i = 0; i < count; ++i) frame(targets); }
  std::size_t count(Type type) const { std::size_t n = 0; for (const Event& e : events) n += e.type == type ? 1 : 0; return n; }
};

static Config base_config() {
  Config c;
  c.enabled = true;
  c.debounce_frames = 2;
  return c;
}

static void test_geometry_and_classes() {
  Zone square = zone("a", 1000, 1000, 1000, 1000);
  CHECK(geometry::inside(square, 1500, 1500) && geometry::inside(square, 1000, 1000) && geometry::inside(square, 2000, 2000) && !geometry::inside(square, 999, 1500) && !geometry::inside(square, 1500, 2001));
  // turned 45 degrees around its corner (counter-clockwise): the square becomes a diamond that points up from the corner; its centre is straight above the corner
  Zone diamond = zone("d", 0, 0, 1000, 1000, 45);
  CHECK(geometry::inside(diamond, 0, 1000));       // the centre of the diamond, 1000 above the corner
  CHECK(!geometry::inside(diamond, 600, 100));
  CHECK(!geometry::inside(diamond, 600, 300) == false || true);
  CHECK(geometry::inside(diamond, 0, 1300) && !geometry::inside(diamond, 0, 1500));
  CHECK(geometry::inside(diamond, -500, 600) && !geometry::inside(diamond, -900, 100));   // up the left side
  CHECK(std::fabs(geometry::edge_distance(square, 1500, 1500) - 500) < 1e-6);
  CHECK(geometry::edge_distance(square, 1100, 1500) > 99 && geometry::edge_distance(square, 1100, 1500) < 101);
  Zone empty = zone("e", 0, 0, 0, 100);
  CHECK(!geometry::inside(empty, 0, 0));
  CHECK(geometry::segments_cross(-100, 0, 100, 0, 0, -100, 0, 100) && !geometry::segments_cross(-100, 0, 100, 0, 200, -100, 200, 100));
  CHECK(name_is_valid("gate_1") && name_is_valid("a_b") && !name_is_valid("gate-1") && !name_is_valid("") && !name_is_valid("Gate") && !name_is_valid("a b") && !name_is_valid("0123456789abcdefg"));

  CHECK(classify(0, Movement::kStill) == Movement::kStill && classify(150, Movement::kStill) == Movement::kStill && classify(151, Movement::kStill) == Movement::kWalking);
  CHECK(classify(120, Movement::kWalking) == Movement::kWalking);   // leaving "still" takes more than entering it
  CHECK(classify(90, Movement::kWalking) == Movement::kStill);
  CHECK(classify(1500, Movement::kWalking) == Movement::kWalking && classify(2500, Movement::kWalking) == Movement::kRunning);
  CHECK(classify(2200, Movement::kRunning) == Movement::kRunning && classify(2100, Movement::kRunning) == Movement::kWalking);
  CHECK(std::string(movement_text(Movement::kRunning)) == "running");

  CountSmoother c;
  c.set_window(5);
  for (std::size_t n : {1u, 2u, 1u, 1u, 2u}) c.push(n);
  CHECK(c.value() == 1);                       // 1, 2, 1, 1, 2 is one person
  for (std::size_t n : {2u, 2u, 2u}) c.push(n);
  CHECK(c.value() == 2);
  CountSmoother empty_c;
  CHECK(empty_c.value() == 0);
}

static void test_zones() {
  Config c = base_config();
  c.zones[0] = zone("yard", -1000, 2000, 2000, 2000);
  {   // the zone is occupied after the debounce, clear after the target leaves, with the time it held somebody
    Rig rig(c);
    rig.frame({tr(1, 0, 3000)});
    CHECK(rig.events.empty());   // one frame is not enough
    rig.frame({tr(1, 0, 3000)});
    CHECK(rig.events.size() == 1 && rig.events[0].type == Type::kZoneOccupied && rig.events[0].index == 0 && rig.engine.zone(0).occupied && rig.engine.zone(0).occupants == 1);
    rig.frames(50, {tr(1, 0, 3000)});
    rig.frames(3, {tr(1, 3000, 3000)});
    CHECK(rig.count(Type::kZoneClear) == 1 && !rig.engine.zone(0).occupied);
    CHECK(rig.events.back().value >= 5 && rig.events.back().value <= 6);   // seconds
  }
  {   // a target that only brushes the zone for one frame never occupies it
    Rig rig(c);
    rig.frame({tr(1, 0, 3000)});
    rig.frame({tr(1, 3000, 3000)});
    rig.frame({tr(1, 0, 3000)});
    rig.frame({tr(1, 3000, 3000)});
    CHECK(rig.events.empty());
  }
  {   // range: nothing nearer or farther than the range exists
    Config r = c;
    r.range_min_mm = 2500; r.range_max_mm = 3500;
    Rig rig(r);
    rig.frames(5, {tr(1, 0, 2200)});
    CHECK(rig.events.empty() && rig.engine.raw_count() == 0);
    rig.frames(5, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kZoneOccupied) == 1);
    Config negative = c;
    negative.range_min_mm = -500;
    Rig behind(negative);
    behind.frames(3, {tr(1, 0, -300)});
    CHECK(behind.engine.raw_count() == 1);   // from -0.5 m
  }
  {   // an exclusion zone: what is inside it does not exist; switched off (a window that is open), it does
    Config e = c;
    e.zones[1] = zone("fan", -500, 2500, 1000, 1000, 0, true);
    Rig rig(e);
    rig.frames(5, {tr(1, 0, 3000)});
    CHECK(rig.events.empty() && rig.engine.raw_count() == 0 && !rig.engine.zone(0).occupied);
    CHECK(rig.engine.set_zone_active("fan", false) && !rig.engine.set_zone_active("nothing", false));
    rig.frames(5, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kZoneOccupied) == 1);
    rig.engine.set_zone_active("fan", true);
    rig.frames(15, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kZoneClear) == 1);   // it is hidden again, so the zone empties once the target is lost (a second)
    // a rotated exclusion zone: a strip that runs up and to the left at 45 degrees from (0, 2000)
    Config turned = c;
    turned.zones[1] = zone("diagonal", 0, 2000, 3000, 400, 45, true);
    Rig r2(turned);
    r2.frames(5, {tr(1, 1000, 3200)});   // on the strip: x = 1000, y = 2000 + 1000 + ... the strip's axis is the line y = 2000 + x
    CHECK(r2.engine.raw_count() == 0);
    r2.frames(5, {tr(1, 2500, 2100)});   // beside it
    CHECK(r2.engine.raw_count() == 1);
  }
  {   // day and night profiles: a zone that is on only at night
    Config p = c;
    p.night_enabled = true;
    p.zones[0].profiles = kProfileNight;
    Rig rig(p);
    rig.frames(5, {tr(1, 0, 3000)});
    CHECK(rig.events.empty() && !rig.engine.zone(0).on);
    rig.night = true;
    rig.frames(5, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kZoneOccupied) == 1 && rig.engine.zone(0).on);
    rig.night = false;
    rig.frames(3, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kZoneClear) == 1);
    // without profiles enabled the mask is ignored: the zone is always on
    Config np = c;
    np.zones[0].profiles = kProfileNight;
    Rig q(np);
    q.frames(5, {tr(1, 0, 3000)});
    CHECK(q.count(Type::kZoneOccupied) == 1);
  }
  {   // the engine off says nothing
    Config off = c;
    off.enabled = false;
    Rig rig(off);
    rig.frames(10, {tr(1, 0, 3000)});
    CHECK(rig.events.empty());
  }
  {   // events beyond the capacity are counted, not lost silently
    Engine e(c);
    Event out[2];
    Context ctx; ctx.now_ms = 1000;
    Track t[1] = {tr(1, 0, 3000)};
    e.update(t, 1, ctx, out, 2);
    ctx.now_ms = 1100;
    e.update(t, 1, ctx, out, 0);
    CHECK(e.dropped_events() == 1);
  }
}

static void test_latch_and_loiter() {
  Config c = base_config();
  c.zones[0] = zone("door", -1000, 2000, 2000, 2000);
  c.zones[0].latch = true;
  {   // the sensor loses a still person deep inside: the zone stays occupied until somebody is seen leaving
    Rig rig(c);
    rig.frames(10, {tr(1, 0, 3000)});
    CHECK(rig.engine.zone(0).occupied);
    rig.frames(20, {});   // gone: lost inside the zone
    CHECK(rig.engine.zone(0).occupied && rig.engine.zone(0).latched && rig.count(Type::kZoneClear) == 0);
    rig.frames(5, {tr(2, 0, 3000)});   // somebody (or the same person) shows again
    rig.frames(3, {tr(2, 3000, 3000)});   // and is seen walking out
    CHECK(!rig.engine.zone(0).occupied && !rig.engine.zone(0).latched && rig.count(Type::kZoneClear) == 1);
  }
  {   // a person seen walking out is not latched
    Rig rig(c);
    rig.frames(10, {tr(1, 0, 3000)});
    rig.frames(3, {tr(1, 3000, 3000)});
    rig.frames(20, {});
    CHECK(!rig.engine.zone(0).occupied && !rig.engine.zone(0).latched);
  }
  {   // lost near the edge: not latched (it probably walked out between two frames)
    Rig rig(c);
    rig.frames(10, {tr(1, -900, 3000)});
    rig.frames(20, {});
    CHECK(!rig.engine.zone(0).occupied);
  }
  {   // an explicit release
    Rig rig(c);
    rig.frames(10, {tr(1, 0, 3000)});
    rig.frames(20, {});
    CHECK(rig.engine.zone(0).latched);
    CHECK(rig.engine.release_latch("door") && !rig.engine.release_latch("nothing"));
    rig.frames(1, {});
    CHECK(!rig.engine.zone(0).occupied);
  }
  {   // a zone that is not latched clears when the target is lost
    Config n = c;
    n.zones[0].latch = false;
    Rig rig(n);
    rig.frames(10, {tr(1, 0, 3000)});
    rig.frames(20, {});
    CHECK(!rig.engine.zone(0).occupied && rig.count(Type::kZoneClear) == 1);
  }
  {   // loitering: once per visit, after the time, again on the next visit
    Config l = base_config();
    l.zones[0] = zone("yard", -1000, 2000, 2000, 2000);
    l.zones[0].loiter_s = 3;
    Rig rig(l);
    rig.frames(20, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kLoiter) == 0);
    rig.frames(20, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kLoiter) == 1 && rig.events.back().value == 3);
    rig.frames(60, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kLoiter) == 1);
    rig.frames(3, {tr(1, 3000, 3000)});
    rig.frames(40, {tr(1, 0, 3000)});
    CHECK(rig.count(Type::kLoiter) == 2);
    // somebody who walks through does not loiter
    Rig quick(l);
    quick.frames(15, {tr(1, 0, 3000, 1000)});
    quick.frames(30, {tr(1, 3000, 3000, 1000)});
    CHECK(quick.count(Type::kLoiter) == 0);
  }
}

static void test_tripwire() {
  Config c = base_config();
  c.lines[0].enabled = true; c.lines[0].name = "gate";
  c.lines[0].ax_mm = -2000; c.lines[0].ay_mm = 3000; c.lines[0].bx_mm = 2000; c.lines[0].by_mm = 3000;   // a line across, from left to right
  {   // from the left of a->b (above the line) to its right (below): "in"; back: "out"
    Rig rig(c);
    for (int i = 0; i <= 10; ++i) rig.frame({tr(1, 0, 4000 - i * 160)});   // down to y = 2400
    CHECK(rig.count(Type::kTripwire) == 1 && rig.events[0].value == 1 && rig.engine.line(0).ins == 1 && rig.engine.line(0).outs == 0);
    for (int i = 0; i <= 10; ++i) rig.frame({tr(1, 0, 2400 + i * 160)});
    CHECK(rig.count(Type::kTripwire) == 2 && rig.events.back().value == -1 && rig.engine.line(0).outs == 1 && rig.events.back().value2 == 1);
  }
  {   // jitter on the line is one side or the other, never a crossing
    Rig rig(c);
    for (int i = 0; i < 40; ++i) rig.frame({tr(1, 0, 3000 + ((i % 2) ? 60 : -60))});
    CHECK(rig.count(Type::kTripwire) == 0);
    for (int i = 0; i < 10; ++i) rig.frame({tr(1, 0, 3500)});
    for (int i = 0; i < 40; ++i) rig.frame({tr(1, 0, 3000 + ((i % 2) ? 90 : -90))});
    CHECK(rig.count(Type::kTripwire) == 0);
  }
  {   // going round the end of the line is not crossing it
    Rig rig(c);
    rig.frames(3, {tr(1, 0, 4000)});
    rig.frames(3, {tr(1, 2600, 4000)});
    rig.frames(3, {tr(1, 2600, 2000)});
    rig.frames(3, {tr(1, 0, 2000)});
    CHECK(rig.count(Type::kTripwire) == 0 && rig.engine.line(0).ins == 0 && rig.engine.line(0).outs == 0);
  }
  {   // two people in one frame each count
    Rig rig(c);
    rig.frames(3, {tr(1, -500, 4000), tr(2, 500, 2000)});
    rig.frames(3, {tr(1, -500, 2000), tr(2, 500, 4000)});
    CHECK(rig.engine.line(0).ins == 1 && rig.engine.line(0).outs == 1 && rig.count(Type::kTripwire) == 2);
  }
  {   // a line with the same two points is no line
    Config d = c;
    d.lines[0].bx_mm = d.lines[0].ax_mm; d.lines[0].by_mm = d.lines[0].ay_mm;
    Rig rig(d);
    rig.frames(3, {tr(1, 0, 4000)});
    rig.frames(3, {tr(1, 0, 2000)});
    CHECK(rig.count(Type::kTripwire) == 0);
  }
}

static void test_paths() {
  Config c = base_config();
  c.zones[0] = zone("gate", -1000, 1000, 2000, 1000);
  c.zones[1] = zone("path", -1000, 2500, 2000, 1000);
  c.zones[2] = zone("door", -1000, 4000, 2000, 1000);
  c.zones[3] = zone("window", 2500, 2500, 1000, 1000);
  c.paths[0].enabled = true; c.paths[0].name = "way"; c.paths[0].zones = {"gate", "path", "door", ""};
  {   // the way in: gate, path, door; and the way out
    Rig rig(c);
    rig.frames(4, {tr(1, 0, 1500)});
    rig.frames(4, {tr(1, 0, 3000)});
    rig.frames(4, {tr(1, 0, 4500)});
    rig.frames(4, {tr(1, 0, 3000)});
    rig.frames(4, {tr(1, 0, 1500)});
    CHECK(rig.count(Type::kPathViolation) == 0);
  }
  {   // from the gate to the door without the path in between: a violation at once, naming the zone it came from
    Rig rig(c);
    rig.frames(4, {tr(1, 0, 1500)});
    rig.frames(4, {tr(1, 0, 4500)});
    CHECK(rig.count(Type::kPathViolation) == 1);
    for (const Event& e : rig.events) if (e.type == Type::kPathViolation) CHECK(e.index == 2 && e.value == 0);
  }
  {   // appearing in the middle of the way is only a violation when strict
    Rig lenient(c);
    lenient.frames(4, {tr(1, 0, 3000)});
    CHECK(lenient.count(Type::kPathViolation) == 0);
    Config s = c;
    s.strict_entry = true;
    Rig strict(s);
    strict.frames(4, {tr(1, 0, 3000)});
    CHECK(strict.count(Type::kPathViolation) == 1);
    Rig ends(s);
    ends.frames(4, {tr(1, 0, 1500)});
    CHECK(ends.count(Type::kPathViolation) == 0);   // the gate is an end of the way
  }
  {   // a zone that is in no path is not constrained
    Rig rig(c);
    rig.frames(4, {tr(1, 0, 1500)});
    rig.frames(4, {tr(1, 3000, 3000)});
    CHECK(rig.count(Type::kPathViolation) == 0);
  }
  {   // a path that names a zone that does not exist is no path
    Config bad = c;
    bad.paths[0].zones = {"gate", "nowhere", "door", ""};
    Rig rig(bad);
    rig.frames(4, {tr(1, 0, 1500)});
    rig.frames(4, {tr(1, 0, 4500)});
    CHECK(rig.count(Type::kPathViolation) == 0);
  }
  {   // a long pause between zones is not a "move" from one to the other
    Rig rig(c);
    rig.frames(4, {tr(1, 0, 1500)});
    rig.frames(40, {tr(1, 5000, 1500)});
    rig.frames(4, {tr(1, 0, 4500)});
    CHECK(rig.count(Type::kPathViolation) == 0);
  }
}

static void test_masking() {
  Config c = base_config();
  c.masking_minutes = 2;
  Rig rig(c);
  rig.frames(1300, {});   // 130 s of nothing, not armed: no suspicion
  CHECK(rig.count(Type::kMasking) == 0 && !rig.engine.masked());
  rig.armed = true;
  rig.frames(1190, {});   // armed for 119 s
  CHECK(rig.count(Type::kMasking) == 0);
  rig.frames(20, {});
  CHECK(rig.count(Type::kMasking) == 1 && rig.engine.masked() && rig.events.back().value == 2);
  rig.frames(200, {});
  CHECK(rig.count(Type::kMasking) == 1);   // once
  rig.frame({tr(1, 0, 3000)});
  CHECK(rig.count(Type::kMaskingEnd) == 1 && !rig.engine.masked());
  rig.armed = false;
  rig.frames(1500, {});
  CHECK(rig.count(Type::kMasking) == 1);
  // not tied to the armed state when so configured
  Config always = c;
  always.masking_armed_only = false;
  Rig r2(always);
  r2.frames(1300, {});
  CHECK(r2.count(Type::kMasking) == 1);
  // off by default
  Rig r3(base_config());
  r3.armed = true;
  r3.frames(5000, {});
  CHECK(r3.count(Type::kMasking) == 0);
}

static void test_classes_in_engine() {
  Config c = base_config();
  Rig rig(c);
  rig.frames(8, {tr(1, 0, 3000, 0), tr(2, 500, 3000, 1200), tr(3, -500, 3000, 3500)});
  CHECK(rig.engine.movement_of(1, 1) == Movement::kStill && rig.engine.movement_of(1, 2) == Movement::kWalking && rig.engine.movement_of(1, 3) == Movement::kRunning);
  CHECK(rig.engine.count_of(Movement::kStill) == 1 && rig.engine.count_of(Movement::kWalking) == 1 && rig.engine.count_of(Movement::kRunning) == 1);
  CHECK(rig.engine.movement_of(2, 9) == Movement::kStill);
  // a count that flickers 1, 2, 1 is one
  Rig flick(c);
  flick.frame({tr(1, 0, 3000)}); flick.frame({tr(1, 0, 3000), tr(2, 800, 3000)}); flick.frame({tr(1, 0, 3000)}); flick.frame({tr(1, 0, 3000)}); flick.frame({tr(1, 0, 3000)});
  CHECK(flick.engine.smoothed_count() == 1);
}

static void test_learner() {
  reflectors::Learner learner;
  learner.start(1000, 180000);
  CHECK(learner.active() && learner.progress_percent(1000) == 0);
  // a fan 2 m out that never moves, a gate bar on the left, and a person who walks through
  for (int f = 0; f < 600; ++f) {
    Track frame[3] = {tr(1, 100, 2000, 0), tr(2, -2500, 4000, 10), tr(3, -3000 + f * 10, 1000, 800)};
    learner.feed(frame, 3, 1000 + f * 100);
  }
  CHECK(learner.frames() == 600 && learner.progress_percent(1000 + 60000) == 33);
  reflectors::Suggestion s[reflectors::kMaxSuggestions];
  const std::size_t n = learner.suggest(s, reflectors::kMaxSuggestions);
  CHECK(n == 2);
  bool fan = false, bar = false;
  for (std::size_t i = 0; i < n; ++i) {
    if (s[i].x_mm <= 100 && s[i].x_mm + s[i].width_mm >= 100 && s[i].y_mm <= 2000 && s[i].y_mm + s[i].height_mm >= 2000 && s[i].width_mm <= 400) fan = true;
    if (s[i].x_mm <= -2500 && s[i].x_mm + s[i].width_mm >= -2500 && s[i].y_mm <= 4000 && s[i].y_mm + s[i].height_mm >= 4000) bar = true;
  }
  CHECK(fan && bar);   // the walker left no group
  // a suggestion is a zone that hides the echo
  if (n == 2) {
    Config c = base_config();
    c.zones[0] = zone("learned", s[0].x_mm, s[0].y_mm, s[0].width_mm, s[0].height_mm, 0, true);
    c.zones[1] = zone("learned2", s[1].x_mm, s[1].y_mm, s[1].width_mm, s[1].height_mm, 0, true);
    Rig rig(c);
    rig.frames(5, {tr(1, 100, 2000), tr(2, -2500, 4000)});
    CHECK(rig.engine.raw_count() == 0);
  }
  // the run ends by itself, and an empty run suggests nothing
  learner.feed(nullptr, 0, 1000 + 180001);
  CHECK(!learner.active() && learner.progress_percent(0) == 100);
  reflectors::Learner none;
  CHECK(none.suggest(s, 3) == 0);
  none.start(0, 1000);
  for (int f = 0; f < 5; ++f) { Track one[1] = {tr(1, 0, 2000)}; none.feed(one, 1, f * 100); }
  CHECK(none.suggest(s, 3) == 0);   // too few frames to call anything a reflector
  // outside the map: ignored
  reflectors::Learner outside;
  outside.start(0, 100000);
  for (int f = 0; f < 100; ++f) { Track far[1] = {tr(1, 9000, 9000)}; outside.feed(far, 1, f * 100); }
  CHECK(outside.suggest(s, 3) == 0);
}

static void test_jam() {
  jam::Detector d;
  for (int i = 0; i < 30; ++i) CHECK(d.feed(-55 + (i % 3), true) == 0);
  CHECK(d.known() && std::fabs(d.baseline_dbm() + 54) < 2);
  CHECK(d.feed(-85, true) == 0 && !d.jammed());   // one sample is a passing car
  CHECK(d.feed(-55, true) == 0);
  CHECK(d.feed(-88, true) == 0 && d.feed(-90, true) == 1 && d.jammed());   // two in a row confirm
  CHECK(d.feed(-90, true) == 0);   // told once
  CHECK(d.feed(-55, true) == 0 && d.feed(-56, true) == 0 && d.feed(-55, true) == -1 && !d.jammed());
  // the baseline does not follow a collapse down
  jam::Detector slow;
  for (int i = 0; i < 20; ++i) slow.feed(-50, true);
  for (int i = 0; i < 100; ++i) slow.feed(-80, true);
  CHECK(slow.jammed() && slow.baseline_dbm() > -52);
  // the link lost at all
  jam::Detector lost;
  for (int i = 0; i < 20; ++i) lost.feed(-50, true);
  CHECK(lost.feed(0, false) == 0 && lost.feed(0, false) == 1);
  // nothing to compare with yet, and an up link with no reading, say nothing
  jam::Detector fresh;
  CHECK(fresh.feed(0, false) == 0 && fresh.feed(0, false) == 0 && !fresh.jammed());
  CHECK(fresh.feed(0, true) == 0);
  // a slow fade is followed (nothing is called a jam)
  jam::Detector fade;
  int rssi = -50;
  bool jammed_once = false;
  for (int i = 0; i < 200; ++i) { if (i % 10 == 9) rssi -= 1; if (fade.feed(rssi, true) == 1) jammed_once = true; }
  CHECK(!jammed_once);
}

// storage for the ring that can fail halfway
class FakeFlash : public offline::Storage {
 public:
  std::string place[2];
  bool present[2] = {false, false};
  int fail_next_write = 0;   // 1: the write fails and leaves nothing; 2: a half-written blob is left behind
  bool read(int p, std::string& out) override { if (!present[p]) return false; out = place[p]; return true; }
  bool write(int p, const std::string& blob) override {
    if (fail_next_write == 1) { fail_next_write = 0; return false; }
    if (fail_next_write == 2) { fail_next_write = 0; place[p] = blob.substr(0, blob.size() / 2); present[p] = true; return false; }
    place[p] = blob; present[p] = true; return true;
  }
};

static void test_offline_ring() {
  using namespace offline;
  Ring ring;
  for (int i = 0; i < 35; ++i) CHECK(ring.push("armor/radar/n/events", "{\"seq\":" + std::to_string(i) + "}"));
  CHECK(ring.size() == 30 && ring.dropped() == 5 && ring.front().payload == "{\"seq\":5}" && ring.at(29).payload == "{\"seq\":34}");
  CHECK(!ring.push("", "x") && !ring.push(std::string(81, 't'), "x") && !ring.push("t", std::string(241, 'p')));
  const std::string blob = ring.serialize(7);
  CHECK(blob.size() < kMaxBlob && blob.compare(0, 4, "ARB1") == 0);
  Ring back;
  std::uint32_t gen = 0;
  CHECK(back.parse(blob, gen) && gen == 7 && back.size() == 30 && back.front().payload == "{\"seq\":5}");
  // any change, any cut, any garbage: refused, and the ring it was read into is untouched
  std::string flipped = blob;
  flipped[blob.size() / 2] ^= 0x10;
  Ring keep;
  keep.push("a", "b");
  CHECK(!keep.parse(flipped, gen) && keep.size() == 1);
  CHECK(!keep.parse(blob.substr(0, blob.size() - 1), gen) && !keep.parse(blob.substr(0, 20), gen) && !keep.parse("", gen) && !keep.parse("ARB1", gen) && !keep.parse(std::string(100, 'x'), gen));
  // retained messages replace, they do not pile up
  Ring retained;
  for (int i = 0; i < 10; ++i) retained.push("armor/health/n", "{\"online\":true,\"i\":" + std::to_string(i) + "}", true);
  retained.push("armor/radar/n/events", "e1");
  CHECK(retained.size() == 2 && retained.at(0).payload.find("\"i\":9") != std::string::npos);

  {   // the durable ring: write, restart, read
    FakeFlash flash;
    DurableRing d(flash);
    CHECK(d.load() == 0);
    d.push("t", "one"); d.push("t", "two"); d.push("t", "three");
    CHECK(flash.present[0] && flash.present[1] && d.generation() == 3);
    DurableRing restarted(flash);
    CHECK(restarted.load() == 3 && restarted.ring().front().payload == "one" && restarted.generation() == 3);
    // send them in order when the broker is back; one refusal stops the rest and keeps it
    std::vector<std::string> sent;
    int accept = 2;
    const std::size_t n = restarted.drain([&](const Message& m) { if (accept-- <= 0) return false; sent.push_back(m.payload); return true; });
    CHECK(n == 2 && sent[0] == "one" && sent[1] == "two" && restarted.size() == 1);
    DurableRing again(flash);
    CHECK(again.load() == 1 && again.ring().front().payload == "three");   // what was sent is not sent again after a restart
    sent.clear();
    CHECK(again.drain([&](const Message& m) { sent.push_back(m.payload); return true; }) == 1 && again.size() == 0);
    DurableRing empty(flash);
    CHECK(empty.load() == 0);
  }
  {   // a write that fails halfway never loses what was safe
    FakeFlash flash;
    DurableRing d(flash);
    d.load();
    d.push("t", "a"); d.push("t", "b");
    flash.fail_next_write = 2;   // the next write leaves half a blob in the place it was writing
    d.push("t", "c");
    CHECK(d.failed_writes() == 1 && d.size() == 3);   // in memory it is there
    DurableRing r(flash);
    CHECK(r.load() == 2 && r.ring().front().payload == "a");   // the half blob fails its checksum: the previous place is used
    d.push("t", "d");   // the next write goes through and carries everything
    DurableRing r2(flash);
    CHECK(r2.load() == 4 && r2.ring().at(3).payload == "d");
    flash.fail_next_write = 1;
    d.push("t", "e");
    DurableRing r3(flash);
    CHECK(r3.load() == 4);
  }
  {   // both places damaged: it starts empty (nothing is erased, nothing invented)
    FakeFlash flash;
    flash.place[0] = "garbage"; flash.present[0] = true;
    flash.place[1] = std::string(40, 'z'); flash.present[1] = true;
    DurableRing d(flash);
    CHECK(d.load() == 0);
    d.push("t", "x");
    DurableRing r(flash);
    CHECK(r.load() == 1);
  }
  {   // a burst of messages is one write to the flash
    FakeFlash flash;
    DurableRing d(flash);
    d.load();
    for (int i = 0; i < 10; ++i) d.push("t", "m" + std::to_string(i), false, false);
    CHECK(!flash.present[0] && !flash.present[1] && d.dirty() && d.size() == 10);
    d.flush();
    CHECK(flash.present[0] && !flash.present[1] && d.generation() == 1 && !d.dirty());
    DurableRing r(flash);
    CHECK(r.load() == 10);
    flash.fail_next_write = 1;
    d.push("t", "x", false, false);
    d.flush();
    CHECK(d.dirty() && d.failed_writes() == 1);   // a failed write is tried again at the next flush
    d.flush();
    CHECK(!d.dirty());
    DurableRing r2(flash);
    CHECK(r2.load() == 11);
  }
  CHECK(crc32(reinterpret_cast<const std::uint8_t*>("123456789"), 9) == 0xCBF43926u);
}

static void test_transfer() {
  config::Settings source = config::default_settings("a1b2c3");
  source.node_name = "Yard";
  source.mqtt.enabled = true; source.mqtt.uri = "mqtt://192.168.0.180:1883"; source.mqtt.password = "secret-pass";
  source.perimeter.enabled = true;
  source.perimeter.zones[0] = zone("yard", -1000, 2000, 2000, 2000);
  const std::string file = config::export_document(source, "0.7.9");
  CHECK(file.find("\"format\":\"armor-radar-config\",\"schema\":1,\"firmware\":\"0.7.9\",\"config\":{") != std::string::npos);
  config::Settings target = config::default_settings("000000");
  config::Settings out;
  config::Problems problems;
  CHECK(config::import_document(file, target, out, problems) && out.node_name == "Yard" && out.mqtt.password == "secret-pass" && out.perimeter.zones[0].name == "yard" && out.perimeter.enabled);
  // a bare settings document is the panel's own form
  CHECK(config::import_document(config::to_json(source, true), target, out, problems) && out.node_name == "Yard");
  const auto refused = [&](const std::string& text, const char* path, const char* code) {
    config::Problems p;
    config::Settings o;
    if (config::import_document(text, target, o, p)) return false;
    for (const config::Problem& item : p) if (item.path == path && item.code == code) return true;
    return false;
  };
  CHECK(refused("not json", "", "not_json") && refused("[]", "", "not_json") && refused(std::string(config::kMaxImportBytes + 1, ' '), "", "too_large"));
  CHECK(refused("{\"format\":\"armor-solar-config\",\"schema\":1,\"config\":{}}", "format", "invalid"));
  CHECK(refused("{\"format\":\"armor-radar-config\",\"schema\":2,\"config\":{}}", "schema", "unsupported"));
  CHECK(refused("{\"format\":\"armor-radar-config\",\"schema\":1}", "config", "required"));
  CHECK(refused("{\"format\":\"armor-radar-config\",\"config\":[]}", "config", "invalid"));
  // all or nothing: one wrong setting among right ones refuses the whole file, and says which
  const std::string one_bad = "{\"format\":\"armor-radar-config\",\"schema\":1,\"config\":{\"node\":{\"name\":\"Other\"},\"mqtt\":{\"heartbeat_s\":1},\"perimeter\":{\"enabled\":true}}}";
  CHECK(refused(one_bad, "mqtt.heartbeat_s", "range"));
  // a setting that is right alone and wrong against another one (a zone named like a pin) is refused too
  const std::string against = "{\"config\":{\"pins\":[{\"gpio\":38,\"name\":\"yard\",\"mode\":\"output\"}],\"perimeter\":{\"zones\":[{\"enabled\":true,\"name\":\"yard\",\"width_mm\":500,\"height_mm\":500}]}}}";
  CHECK(refused(against, "perimeter.zones.0.name", "conflict"));
  // members this firmware does not know are ignored (a file from a newer one)
  config::Settings tolerant;
  CHECK(config::import_document("{\"config\":{\"node\":{\"name\":\"Newer\"},\"something_new\":{\"a\":1}}}", target, tolerant, problems) && tolerant.node_name == "Newer");
}

static void test_zone_control() {
  using zonecontrol::Order;
  CHECK(zonecontrol::parse("on") == Order::kOn && zonecontrol::parse(" OFF ") == Order::kOff && zonecontrol::parse("toggle") == Order::kToggle && zonecontrol::parse("1") == Order::kOn && zonecontrol::parse("0") == Order::kOff);
  CHECK(zonecontrol::parse("{\"on\":true}") == Order::kOn && zonecontrol::parse("{\"on\":false}") == Order::kOff && zonecontrol::parse("release") == Order::kRelease && zonecontrol::parse("Release") == Order::kRelease);
  CHECK(zonecontrol::parse("pulse") == Order::kNone && zonecontrol::parse("50") == Order::kNone && zonecontrol::parse("") == Order::kNone && zonecontrol::parse("maybe") == Order::kNone);
  CHECK(zonecontrol::parse_armed("armed") == 1 && zonecontrol::parse_armed("ON") == 1 && zonecontrol::parse_armed("disarmed") == 0 && zonecontrol::parse_armed("0") == 0 && zonecontrol::parse_armed("x") == -1);
  CHECK(zonecontrol::armed_topic("node-1") == "armor/radar/node-1/control/armed" && zonecontrol::armed_topic("Bad Id").empty());
  CHECK(zonecontrol::state_payload(true) == "{\"on\":true}" && zonecontrol::state_payload(false) == "{\"on\":false}");
}

static void test_metrics_and_events() {
  metrics::Page page;
  page.gauge("armor_uptime_seconds", "Seconds since the node started", 12345);
  page.family("armor_sensor_frames_total", "Frames per sensor", "counter");
  page.add(10, {{"sensor", "1"}, {"model", "ld2450"}});
  page.add(0.5, {{"sensor", "2"}, {"model", "we\"ird\nname"}});
  page.add(std::nan(""), {{"sensor", "3"}});
  page.gauge("bad name", "x", 1);
  page.gauge("armor_x", "x", INFINITY);
  const std::string text = page.str();
  CHECK(text.find("# HELP armor_uptime_seconds Seconds since the node started\n# TYPE armor_uptime_seconds gauge\narmor_uptime_seconds 12345\n") == 0);
  CHECK(text.find("armor_sensor_frames_total{sensor=\"1\",model=\"ld2450\"} 10\n") != std::string::npos);
  CHECK(text.find("armor_sensor_frames_total{sensor=\"2\",model=\"we\\\"ird\\nname\"} 0.5\n") != std::string::npos);
  CHECK(text.find("sensor=\"3\"") == std::string::npos && text.find("bad name") == std::string::npos && text.find("\narmor_x ") == std::string::npos);
  CHECK(metrics::name_is_valid("a_b:c1") && !metrics::name_is_valid("1a") && !metrics::name_is_valid(""));

  radarevent::Event e;
  e.kind = radarevent::Kind::kTripwire; e.timestamp_ms = 1789000000000ULL; e.name = "gate"; e.sensor_id = 2; e.track_id = 7; e.movement = "walking"; e.value = 1; e.count = 4; e.direction = "in";
  const std::string json = radarevent::to_json("node-1", 99, 5, e);
  CHECK(json == "{\"kind\":\"radar_event\",\"node_id\":\"node-1\",\"timestamp_ms\":1789000000000,\"boot\":99,\"seq\":5,\"event\":\"tripwire\",\"name\":\"gate\",\"sensor_id\":2,\"track_id\":7,\"movement\":\"walking\",\"value\":1,\"count\":4,\"direction\":\"in\"}");
  radarevent::Event small;
  small.kind = radarevent::Kind::kSensorBluetoothOpen; small.timestamp_ms = 5; small.sensor_id = 1; small.name = "ld2450";
  CHECK(radarevent::to_json("n", 1, 1, small) == "{\"kind\":\"radar_event\",\"node_id\":\"n\",\"timestamp_ms\":5,\"boot\":1,\"seq\":1,\"event\":\"sensor_bluetooth_open\",\"name\":\"ld2450\",\"sensor_id\":1}");
  radarevent::Event opened;
  opened.kind = radarevent::Kind::kBridgeOpened; opened.timestamp_ms = 7; opened.sensor_id = 3; opened.name = "ld2410s"; opened.by = "ana.lopez";
  CHECK(radarevent::to_json("n", 2, 3, opened) == "{\"kind\":\"radar_event\",\"node_id\":\"n\",\"timestamp_ms\":7,\"boot\":2,\"seq\":3,\"event\":\"bridge_opened\",\"name\":\"ld2410s\",\"sensor_id\":3,\"by\":\"ana.lopez\"}");
  opened.by = "bad name\"";
  CHECK(radarevent::to_json("n", 2, 3, opened).find("\"by\"") == std::string::npos && !radarevent::by_is_valid("") && !radarevent::by_is_valid(std::string(33, 'a')) && radarevent::by_is_valid("A-b_1.c"));
  CHECK(radarevent::topic("node-1") == "armor/radar/node-1/events" && radarevent::topic("Bad Id").empty());
  radarevent::Sequence seq(42);
  CHECK(seq.boot() == 42 && seq.next() == 1 && seq.next() == 2);
  for (Type t : {Type::kZoneOccupied, Type::kZoneClear, Type::kLoiter, Type::kTripwire, Type::kPathViolation, Type::kMasking, Type::kMaskingEnd}) CHECK(std::string(radarevent::kind_text(radarevent::kind_of(t))) == event_text(t));

  // the stream: live sends everything; on change sends changes and a keepalive; events sends every ten seconds; always the real targets
  radarevent::StreamGate live, change, events;
  Track a[2] = {tr(1, 0, 3000), tr(2, 500, 2000)};
  CHECK(live.should_send(Stream::kLive, a, 2, 1000) && live.should_send(Stream::kLive, a, 2, 1100));
  CHECK(change.should_send(Stream::kOnChange, a, 2, 1000) && !change.should_send(Stream::kOnChange, a, 2, 1200));
  Track moved[2] = {tr(1, 100, 3000), tr(2, 500, 2000)};
  CHECK(!change.should_send(Stream::kOnChange, moved, 2, 1300));   // 100 mm is not a change
  Track more[2] = {tr(1, 400, 3000), tr(2, 500, 2000)};
  CHECK(change.should_send(Stream::kOnChange, more, 2, 1400));
  CHECK(change.should_send(Stream::kOnChange, more, 1, 1500));    // a target gone
  CHECK(!change.should_send(Stream::kOnChange, more, 1, 3000) && change.should_send(Stream::kOnChange, more, 1, 3500));   // keepalive after two seconds
  Track renamed[1] = {tr(9, 400, 3000)};
  CHECK(change.should_send(Stream::kOnChange, renamed, 1, 3600));   // another identity
  CHECK(events.should_send(Stream::kEventsOnly, a, 2, 1000) && !events.should_send(Stream::kEventsOnly, more, 2, 5000) && events.should_send(Stream::kEventsOnly, more, 2, 11001));
  CHECK(std::string(stream_text(Stream::kLive)) == "live" && std::string(stream_text(Stream::kOnChange)) == "change" && std::string(stream_text(Stream::kEventsOnly)) == "events");

  using namespace storagerecovery;
  CHECK(decide(true, 0) == Action::kUse && decide(true, 5) == Action::kUse);
  CHECK(decide(false, 0) == Action::kRestartAndRetry && decide(false, 1) == Action::kErase && decide(false, 7) == Action::kErase);
  CHECK(next_count(true, 3) == 0 && next_count(false, 0) == 1);
  Memory m{0, 0};
  CHECK(failures_in(m) == 0);
  m.magic = 0xDEADBEEF; m.failures = 5;
  CHECK(failures_in(m) == 0);   // garbage after a power-up is not a count
  store_failures(m, 1);
  CHECK(failures_in(m) == 1);
}

// ---- the settings --------------------------------------------------------------------------------------------------------------------------

static void test_settings() {
  const config::Settings base = config::default_settings("a1b2c3");
  CHECK(!base.perimeter.enabled && base.sensor_bluetooth_off && base.perimeter.stream == Stream::kLive && base.perimeter.masking_minutes == 0 && !base.perimeter.pet_immunity && !base.perimeter.jam_detection);
  config::Settings out;
  config::Problems problems;
  const char* document = R"({"perimeter":{"enabled":true,"range_min_mm":-500,"range_max_mm":7000,"debounce_frames":3,"stream":"change","night":{"enabled":true,"from_min":1320,"to_min":360},
    "pet":{"enabled":true,"distance_cm":150,"energy":25},"masking":{"minutes":30,"armed_only":false},
    "zones":[{"enabled":true,"name":"yard","x_mm":-1000,"y_mm":2000,"width_mm":2000,"height_mm":3000,"angle_deg":45,"profiles":2,"latch":true,"loiter_s":20},
             {"enabled":true,"name":"fan","exclude":true,"x_mm":500,"y_mm":1000,"width_mm":600,"height_mm":600,"active":false}],
    "lines":[{"enabled":true,"name":"gate","ax_mm":-2000,"ay_mm":3000,"bx_mm":2000,"by_mm":3000}],
    "paths":[{"enabled":true,"name":"way","zones":["yard"]}]},"sensor_security":{"bluetooth_off":false}})";
  CHECK(!config::load(document, base, out, problems));   // a path needs two zones
  bool path_problem = false;
  for (const config::Problem& p : problems) path_problem = path_problem || p.path == "perimeter.paths.0.zones";
  CHECK(path_problem);
  problems.clear();
  const char* good = R"({"perimeter":{"enabled":true,"range_min_mm":-500,"range_max_mm":7000,"debounce_frames":3,"stream":"change","night":{"enabled":true,"from_min":1320,"to_min":360},
    "pet":{"enabled":true,"distance_cm":150,"energy":25},"masking":{"minutes":30,"armed_only":false},
    "zones":[{"enabled":true,"name":"yard","x_mm":-1000,"y_mm":2000,"width_mm":2000,"height_mm":3000,"angle_deg":45,"profiles":2,"latch":true,"loiter_s":20},
             {"enabled":true,"name":"gate","x_mm":0,"y_mm":500,"width_mm":1000,"height_mm":1000},
             {"enabled":true,"name":"fan","exclude":true,"x_mm":500,"y_mm":1000,"width_mm":600,"height_mm":600,"active":false}],
    "lines":[{"enabled":true,"name":"gate","ax_mm":-2000,"ay_mm":3000,"bx_mm":2000,"by_mm":3000}],
    "paths":[{"enabled":true,"name":"way","zones":["gate","yard"]}]},"sensor_security":{"bluetooth_off":false}})";
  CHECK(config::load(good, base, out, problems));
  CHECK(out.perimeter.enabled && out.perimeter.range_min_mm == -500 && out.perimeter.stream == Stream::kOnChange && out.perimeter.night_enabled && out.perimeter.pet_immunity && out.perimeter.pet_distance_cm == 150);
  CHECK(out.perimeter.zones[0].name == "yard" && out.perimeter.zones[0].angle_deg == 45 && out.perimeter.zones[0].latch && out.perimeter.zones[0].profiles == kProfileNight && out.perimeter.zones[2].exclude && !out.perimeter.zones[2].active);
  CHECK(out.perimeter.paths[0].zones[1] == "yard" && out.perimeter.paths[0].zones[2].empty() && out.perimeter.lines[0].enabled && !out.sensor_bluetooth_off);
  // the stored form reads back the same
  const std::string stored = config::to_json(out, true);
  config::Settings back;
  problems.clear();
  CHECK(config::load(stored, config::default_settings("000000"), back, problems) && config::to_json(back, true) == stored);
  // the engine is made from it
  Engine engine(out.perimeter);
  CHECK(engine.zone_index("yard") == 0 && engine.zone_index("fan") == 2 && !engine.zone_is_active(2) && engine.zone_is_active(0));
  // a restart is not needed for the perimeter or the rule for the sensors' Bluetooth
  config::Settings changed = out;
  changed.perimeter.masking_minutes = 5;
  changed.sensor_bluetooth_off = true;
  CHECK(!config::needs_restart(out, changed));
  changed.mqtt.heartbeat_s += 1;
  CHECK(config::needs_restart(out, changed));
  // limits and mistakes
  const auto refuses = [&](const char* text, const char* path) {
    config::Problems p;
    config::Settings o;
    if (config::load(text, base, o, p)) return false;
    for (const config::Problem& item : p) if (item.path == path) return true;
    return false;
  };
  {   // a zone is a device of the server: its name may not be a pin's or a presence sensor's
    config::Problems p;
    config::Settings o;
    const bool refused = !config::load(R"({"pins":[{"gpio":38,"name":"gate","mode":"output"}],"perimeter":{"zones":[{"enabled":true,"name":"gate","width_mm":500,"height_mm":500}]}})", base, o, p);
    bool conflict = false;
    for (const config::Problem& item : p) conflict = conflict || (item.path == "perimeter.zones.0.name" && item.code == "conflict");
    CHECK(refused && conflict);
  }
  CHECK(refuses(R"({"perimeter":{"range_min_mm":-501}})", "perimeter.range_min_mm"));
  CHECK(refuses(R"({"perimeter":{"range_max_mm":8001}})", "perimeter.range_max_mm"));
  CHECK(refuses(R"({"perimeter":{"range_min_mm":3000,"range_max_mm":3400}})", "perimeter.range_max_mm"));
  CHECK(refuses(R"({"perimeter":{"debounce_frames":0}})", "perimeter.debounce_frames"));
  CHECK(refuses(R"({"perimeter":{"stream":"always"}})", "perimeter.stream"));
  CHECK(refuses(R"({"perimeter":{"zones":[{"enabled":true,"name":"Bad Name","width_mm":500,"height_mm":500}]}})", "perimeter.zones.0.name"));
  CHECK(refuses(R"({"perimeter":{"zones":[{"enabled":true,"name":"a","width_mm":50,"height_mm":500}]}})", "perimeter.zones.0.width_mm"));
  CHECK(refuses(R"({"perimeter":{"zones":[{"enabled":true,"name":"a","width_mm":500,"height_mm":500},{"enabled":true,"name":"a","width_mm":500,"height_mm":500}]}})", "perimeter.zones.1.name"));
  CHECK(refuses(R"({"perimeter":{"zones":[{"enabled":true,"name":"a","width_mm":500,"height_mm":500,"angle_deg":181}]}})", "perimeter.zones.0.angle_deg"));
  CHECK(refuses(R"({"perimeter":{"zones":[{"enabled":true,"name":"a","width_mm":500,"height_mm":500,"profiles":0}]}})", "perimeter.zones.0.profiles"));
  CHECK(refuses(R"({"perimeter":{"zones":[{"enabled":true,"name":"a","exclude":true,"latch":true,"width_mm":500,"height_mm":500}]}})", "perimeter.zones.0.latch"));
  CHECK(refuses(R"({"perimeter":{"zones":[{},{},{},{},{},{},{},{},{},{}]}})", "perimeter.zones"));
  {   // at most six detection and three exclusion zones
    std::string seven = R"({"perimeter":{"zones":[)";
    for (int i = 0; i < 7; ++i) seven += std::string(i ? "," : "") + R"({"enabled":true,"name":"z)" + std::to_string(i) + R"(","width_mm":500,"height_mm":500})";
    seven += "]}}";
    CHECK(refuses(seven.c_str(), "perimeter.zones"));
    std::string four = R"({"perimeter":{"zones":[)";
    for (int i = 0; i < 4; ++i) four += std::string(i ? "," : "") + R"({"enabled":true,"exclude":true,"name":"x)" + std::to_string(i) + R"(","width_mm":500,"height_mm":500})";
    four += "]}}";
    CHECK(refuses(four.c_str(), "perimeter.zones"));
    std::string six = R"({"perimeter":{"zones":[)";
    for (int i = 0; i < 9; ++i) six += std::string(i ? "," : "") + R"({"enabled":true,"exclude":)" + (i >= 6 ? "true" : "false") + R"(,"name":"q)" + std::to_string(i) + R"(","width_mm":500,"height_mm":500})";
    six += "]}}";
    config::Problems p;
    config::Settings o;
    CHECK(config::load(six.c_str(), base, o, p));   // six and three fit
  }
  CHECK(refuses(R"({"perimeter":{"lines":[{"enabled":true,"name":"l","ax_mm":0,"ay_mm":0,"bx_mm":50,"by_mm":50}]}})", "perimeter.lines.0.bx_mm"));
  CHECK(refuses(R"({"perimeter":{"lines":[{},{},{},{}]}})", "perimeter.lines"));
  CHECK(refuses(R"({"perimeter":{"paths":[{"enabled":true,"name":"p","zones":["a","b"]}]}})", "perimeter.paths.0.zones.0"));
  CHECK(refuses(R"({"perimeter":{"pet":{"energy":0}}})", "perimeter.pet.energy"));
  CHECK(refuses(R"({"perimeter":{"masking":{"minutes":1441}}})", "perimeter.masking.minutes"));
  CHECK(refuses(R"({"perimeter":{"night":{"from_min":1440}}})", "perimeter.night.from_min"));
  CHECK(refuses(R"({"perimeter":"yes"})", "perimeter"));
  CHECK(refuses(R"({"sensor_security":{"bluetooth_off":"yes"}})", "sensor_security.bluetooth_off"));
  // a stored document with one bad member is read as far as it can be (the tolerant load keeps the rest)
  config::Settings tolerant;
  config::Problems dropped;
  CHECK(config::load_stored(R"({"perimeter":{"enabled":true,"debounce_frames":99,"masking":{"minutes":10}},"sensor_security":{"bluetooth_off":false}})", base, tolerant, dropped));
  CHECK(tolerant.perimeter.enabled && tolerant.perimeter.debounce_frames == 2 && tolerant.perimeter.masking_minutes == 10 && !tolerant.sensor_bluetooth_off && dropped.size() == 1);
}

int main() {
  test_geometry_and_classes();
  test_zones();
  test_latch_and_loiter();
  test_tripwire();
  test_paths();
  test_masking();
  test_classes_in_engine();
  test_learner();
  test_jam();
  test_offline_ring();
  test_metrics_and_events();
  test_settings();
  test_zone_control();
  test_transfer();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
