// ARMOR-NODE-RADAR - host tests of the assisted alignment (main/core/assisted_calibration.hpp) against a simulated radar of known heading and position.
// No radar was used. Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cmath>
#include <cstdio>
#include <vector>

#include "../main/core/assisted_calibration.hpp"

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

using namespace armor::calibration;

static const double kPi = 3.14159265358979323846;

// What a radar with heading `yaw_deg` and position (ox, oy) reads when a person stands at the mark (ref_x, ref_y): the inverse of shared = R(yaw) * raw + offset.
static RefPoint simulate(double ref_x, double ref_y, double yaw_deg, double ox, double oy, double noise_x = 0, double noise_y = 0) {
  const double yaw = yaw_deg * kPi / 180.0;
  const double c = std::cos(yaw), s = std::sin(yaw);
  const double dx = ref_x - ox, dy = ref_y - oy;
  RefPoint p;
  p.ref_x = ref_x; p.ref_y = ref_y;
  p.raw_x = c * dx + s * dy + noise_x;
  p.raw_y = -s * dx + c * dy + noise_y;
  return p;
}

static void test_alignment() {
  std::vector<RefPoint> clean = {simulate(0, 2000, 30, 1500, -400), simulate(2000, 3000, 30, 1500, -400), simulate(-1500, 4000, 30, 1500, -400)};
  Alignment a = align(clean.data(), clean.size());
  CHECK(a.ok && std::fabs(a.yaw_deg - 30.0) < 0.01 && std::fabs(a.offset_x_mm - 1500.0) < 1.0 && std::fabs(a.offset_y_mm + 400.0) < 1.0 && a.rms_mm < 1.0);
  CHECK(a.yaw_setting == 30 && a.offset_x_setting == 1500 && a.offset_y_setting == -400 && a.in_range);

  // a radar turned the other way, with two marks only
  std::vector<RefPoint> two = {simulate(500, 1000, -120, -800, 200), simulate(-1000, 3500, -120, -800, 200)};
  a = align(two.data(), two.size());
  CHECK(a.ok && a.yaw_setting == -120 && a.offset_x_setting == -800 && a.offset_y_setting == 200);

  // readings with some noise (about 40 mm) still give an alignment within a degree or two
  std::vector<RefPoint> noisy = {simulate(0, 2000, 90, 0, 0, 35, -20), simulate(2000, 3000, 90, 0, 0, -30, 40), simulate(-1500, 4000, 90, 0, 0, 25, 10), simulate(1000, 1000, 90, 0, 0, -15, -35)};
  a = align(noisy.data(), noisy.size());
  CHECK(a.ok && std::fabs(a.yaw_deg - 90.0) < 2.0 && a.rms_mm > 5.0 && a.rms_mm < 80.0);

  // what cannot be aligned
  CHECK(!align(clean.data(), 1).ok && !align(nullptr, 3).ok && !align(clean.data(), 0).ok);
  std::vector<RefPoint> close = {simulate(0, 2000, 30, 0, 0), simulate(100, 2000, 30, 0, 0)};
  CHECK(!align(close.data(), close.size()).ok);                              // the marks are 100 mm apart: the heading is not known
  std::vector<RefPoint> wrong = clean;
  wrong[1].raw_x += 1500;                                                    // one reading is far off its mark
  a = align(wrong.data(), wrong.size());
  CHECK(!a.ok && a.rms_mm > 250.0);
  a = align(wrong.data(), wrong.size(), 2000.0);                             // with a wider limit the same points are taken
  CHECK(a.ok);

  // a result that does not fit the settings (the radar is 6 m from the origin) is flagged
  std::vector<RefPoint> far = {simulate(0, 2000, 0, 6000, 0), simulate(2000, 3000, 0, 6000, 0)};
  a = align(far.data(), far.size());
  CHECK(a.ok && !a.in_range && a.offset_x_setting == 6000);
  std::vector<RefPoint> far_y = {simulate(0, 2000, 0, 0, -6000), simulate(2000, 3000, 0, 0, -6000)};
  a = align(far_y.data(), far_y.size());
  CHECK(a.ok && !a.in_range && a.offset_y_setting == -6000);
}

static void feed(AlignSession& session, const RefPoint& point, std::size_t samples) {
  for (std::size_t i = 0; i < samples; ++i) session.sample(point.raw_x + (i % 2 == 0 ? 10 : -10), point.raw_y);   // 10 mm of jitter around the mean
}

static void test_session() {
  AlignSession session;
  session.sample(1, 1);                                   // nothing is being read: ignored
  CHECK(!session.finish_point() && session.points() == 0);
  const RefPoint one = simulate(0, 2000, 45, 300, 300), two = simulate(2000, 3000, 45, 300, 300), three = simulate(-1500, 4000, 45, 300, 300);
  CHECK(session.begin_point(one.ref_x, one.ref_y));
  CHECK(!session.begin_point(5, 5));                      // one is still being read
  feed(session, one, 5);
  CHECK(!session.finish_point());                         // too few readings: the person is asked to stay
  feed(session, one, 15);
  CHECK(session.finish_point() && session.points() == 1);
  CHECK(session.begin_point(two.ref_x, two.ref_y));
  feed(session, two, 30);
  CHECK(session.finish_point(30) && session.points() == 2);
  Alignment a = session.result();
  CHECK(a.ok && a.yaw_setting == 45 && std::abs(a.offset_x_setting - 300) <= 5 && std::abs(a.offset_y_setting - 300) <= 5);
  CHECK(session.begin_point(three.ref_x, three.ref_y));
  feed(session, three, 25);
  CHECK(session.finish_point() && session.points() == 3);
  for (int i = 0; i < 3; ++i) { CHECK(session.begin_point(0, 1000.0 * (i + 1))); feed(session, simulate(0, 1000.0 * (i + 1), 45, 300, 300), 20); CHECK(session.finish_point()); }
  CHECK(session.points() == 6 && !session.begin_point(1, 1));   // six marks are the most
  session.reset();
  CHECK(session.points() == 0 && !session.result().ok && session.begin_point(0, 0));
}

static armor::Track target(std::uint8_t sensor, std::uint8_t id, int x, int y) {
  armor::Track out{};
  out.sensor_id = sensor; out.track_id = id; out.x_mm = static_cast<std::int16_t>(x); out.y_mm = static_cast<std::int16_t>(y);
  return out;
}

static void test_feed_from_tracks() {
  AlignSession session;
  const armor::Track one[] = {target(1, 1, 100, 2000)};
  CHECK(!session.feed_tracks(one, 1, 1));                  // no mark is open
  CHECK(session.begin_point(0, 2000) && session.reading() && session.samples() == 0);
  CHECK(session.feed_tracks(one, 1, 1) && session.samples() == 1);
  CHECK(!session.feed_tracks(one, 1, 2));                  // another radar: left out
  CHECK(!session.feed_tracks(nullptr, 0, 1) && !session.feed_tracks(one, 0, 1));
  const armor::Track two[] = {target(1, 1, 100, 2000), target(1, 2, -900, 3000), target(2, 1, 5, 5)};
  CHECK(!session.feed_tracks(two, 3, 1) && session.samples() == 1);   // two people on the radar: no telling which one
  CHECK(session.feed_tracks(two, 3, 2) && session.samples() == 2);    // one on the other radar is still one
  const armor::Track invalid[] = {target(1, 0, 100, 2000), target(0, 1, 100, 2000), target(9, 1, 100, 2000)};
  CHECK(!session.feed_tracks(invalid, 3, 1) && session.samples() == 2);
  for (int i = 0; i < 25; ++i) session.feed_tracks(one, 1, 1);
  CHECK(session.finish_point() && !session.reading() && session.points() == 1);
}

int main() {
  test_alignment();
  test_session();
  test_feed_from_tracks();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
