// ARMOR-NODE-RADAR - host tests for the sensor family: identity (firmware, Bluetooth address), the quirks of some firmware versions, closing the sensors' own Bluetooth with a fake sensor,
// the transparent serial bridge, the hardened LD2450 parser and the tracking.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The frames marked "manual" are the worked examples of the manufacturers' documents.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../main/core/frame_framer.hpp"
#include "../main/core/ld2450.hpp"
#include "../main/core/ld2450_command.hpp"
#include "../main/core/pet_filter.hpp"
#include "../main/core/presence.hpp"
#include "../main/core/radar_calibration.hpp"
#include "../main/core/radar_report.hpp"
#include "../main/core/sensor_bluetooth.hpp"
#include "../main/core/sensor_identity.hpp"
#include "../main/core/serial_bridge.hpp"
#include "../main/core/track_stabiliser.hpp"

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
using Bytes = std::vector<std::uint8_t>;

// An acknowledgement as the node reads it: the bytes go through the real parser.
static ld2450cmd::Ack ack_of(const Bytes& frame) {
  ld2450cmd::AckParser parser;
  ld2450cmd::Ack ack;
  bool got = false;
  for (std::uint8_t b : frame) got = parser.feed(b, ack) || got;
  if (!got) { ack = ld2450cmd::Ack{}; ack.command = 0xFFFF; }
  return ack;
}

// ---- firmware and Bluetooth address -----------------------------------------------------------------------------------------------------

static void test_identity() {
  // manual (LD2450 1.03, 2.2.6): V1.02.22062416
  const sensorid::Firmware a = sensorid::firmware_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0C, 0x00, 0xA0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x16, 0x24, 0x06, 0x22, 0x04, 0x03, 0x02, 0x01}));
  CHECK(a.known && a.text == "1.02.22062416" && a.major == 0x0102 && a.type == 0);
  // manual (LD2412 1.05, 2.2.15): V1.10.24041810, type 0x2412
  const sensorid::Firmware b = sensorid::firmware_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0C, 0x00, 0xA0, 0x01, 0x00, 0x00, 0x12, 0x24, 0x10, 0x01, 0x10, 0x18, 0x04, 0x24, 0x04, 0x03, 0x02, 0x01}));
  CHECK(b.known && b.text == "1.10.24041810" && b.type == 0x2412);
  // a sensor that does not know its own version (the LD2412 V1.24 answers zeros) or answers ones: unknown, never a number
  const sensorid::Firmware zeros = sensorid::firmware_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0C, 0x00, 0xA0, 0x01, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0x04, 0x03, 0x02, 0x01}));
  CHECK(!zeros.known && zeros.text.empty());
  const sensorid::Firmware ones = sensorid::firmware_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0C, 0x00, 0xA0, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0x03, 0x02, 0x01}));
  CHECK(!ones.known);
  CHECK(!sensorid::firmware_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xA0, 0x01, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01})).known);   // refused
  CHECK(!sensorid::firmware_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x06, 0x00, 0xA0, 0x01, 0x00, 0x00, 0x01, 0x02, 0x04, 0x03, 0x02, 0x01})).known);   // too short
  CHECK(!sensorid::firmware_from_ack(ld2450cmd::Ack{}).known);

  // manual (LD2450 1.03, 2.2.11): the address 8F 27 2E B8 0F 65
  const sensorid::Mac open = sensorid::mac_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0A, 0x00, 0xA5, 0x01, 0x00, 0x00, 0x8F, 0x27, 0x2E, 0xB8, 0x0F, 0x65, 0x04, 0x03, 0x02, 0x01}));
  CHECK(open.known && open.text == "8F:27:2E:B8:0F:65" && open.bluetooth == sensorid::Bluetooth::kOpen);
  // 08:05:04:03:02:01 is what a sensor answers while its Bluetooth is off: that is the state, not an address
  const sensorid::Mac closed = sensorid::mac_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0A, 0x00, 0xA5, 0x01, 0x00, 0x00, 0x08, 0x05, 0x04, 0x03, 0x02, 0x01, 0x04, 0x03, 0x02, 0x01}));
  CHECK(closed.known && closed.bluetooth == sensorid::Bluetooth::kClosed && closed.text.empty());
  const sensorid::Mac none = sensorid::mac_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0A, 0x00, 0xA5, 0x01, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0x04, 0x03, 0x02, 0x01}));
  CHECK(!none.known && none.bluetooth == sensorid::Bluetooth::kUnknown);
  const sensorid::Mac typed = sensorid::mac_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0B, 0x00, 0xA5, 0x01, 0x00, 0x00, 0x00, 0x8F, 0x27, 0x2E, 0xB8, 0x0F, 0x65, 0x04, 0x03, 0x02, 0x01}));   // the LD2410C text: a type byte first
  CHECK(typed.known && typed.text == "8F:27:2E:B8:0F:65");
  CHECK(!sensorid::mac_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xA5, 0x01, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01})).known);   // refused
  CHECK(!sensorid::mac_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xA5, 0x01, 0x00, 0x00, 1, 2, 3, 0x04, 0x03, 0x02, 0x01})).known);   // three bytes
  CHECK(std::string(sensorid::bluetooth_text(sensorid::Bluetooth::kOpen)) == "open" && std::string(sensorid::bluetooth_text(sensorid::Bluetooth::kUnknown)) == "unknown");

  // LD2410S: the serial number (manual 2.2.6) and a version that arrives without a status word
  const sensorid::Firmware s = sensorid::firmware_from_ld2410s_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01}));
  CHECK(s.known && s.text == "0100.0000.0000");
  CHECK(sensorid::serial_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0E, 0x00, 0x11, 0x01, 0x00, 0x00, 0x08, 0x00, '1', '2', '3', '4', '5', '6', '7', '8', 0x04, 0x03, 0x02, 0x01})) == "12345678");
  CHECK(sensorid::serial_from_ack(ack_of({0xFD, 0xFC, 0xFB, 0xFA, 0x0E, 0x00, 0x11, 0x01, 0x00, 0x00, 0x08, 0x00, '1', '2', 3, '4', '5', '6', '7', '8', 0x04, 0x03, 0x02, 0x01})).empty());   // not printable

  // which models have a radio of their own to close
  using sensors::Model;
  CHECK(sensorid::has_own_bluetooth(Model::kLd2450) && sensorid::has_own_bluetooth(Model::kLd2410) && sensorid::has_own_bluetooth(Model::kLd2412));
  CHECK(!sensorid::has_own_bluetooth(Model::kLd2461) && !sensorid::has_own_bluetooth(Model::kLd2410s) && !sensorid::has_own_bluetooth(Model::kMr24hpc1));
  CHECK(sensorid::initial_bluetooth(Model::kLd2461) == sensorid::Bluetooth::kNone && sensorid::initial_bluetooth(Model::kLd2450) == sensorid::Bluetooth::kUnknown);
}

static void test_ld2412_quirks() {
  using sensors::Model;
  sensorid::Firmware v124, v126, v136, v110, unknown;
  v124.known = true; v124.major = 0x0124;
  v126.known = true; v126.major = 0x0126;
  v136.known = true; v136.major = 0x0136;
  v110.known = true; v110.major = 0x0110;
  CHECK(sensorid::ld2412_level(Model::kLd2412, v124) == sensorid::Ld2412Level::kBad && sensorid::ld2412_level(Model::kLd2412, v126) == sensorid::Ld2412Level::kBad);
  CHECK(sensorid::ld2412_level(Model::kLd2412, v136) == sensorid::Ld2412Level::kGood);
  CHECK(sensorid::ld2412_level(Model::kLd2412, v110) == sensorid::Ld2412Level::kUnknown && sensorid::ld2412_level(Model::kLd2412, unknown) == sensorid::Ld2412Level::kUnknown);
  CHECK(sensorid::ld2412_level(Model::kLd2450, v124) == sensorid::Ld2412Level::kNotLd2412);
  for (const sensorid::Firmware* f : {&v124, &v126, &v110, &unknown}) {
    const sensorid::Ld2412Policy p = sensorid::ld2412_policy(Model::kLd2412, *f);
    CHECK(!p.may_set_baud && !p.may_set_resolution && !p.trust_engineering_ack && p.verify_gates && p.warn && p.verify_after_ms == 40000);
  }
  const sensorid::Ld2412Policy good = sensorid::ld2412_policy(Model::kLd2412, v136);
  CHECK(good.may_set_resolution && good.trust_engineering_ack && !good.verify_gates && !good.warn);
  CHECK(!sensorid::ld2412_policy(Model::kLd2450, v124).warn && !sensorid::ld2412_policy(Model::kLd2450, v124).verify_gates);
  // the node never changes any sensor's speed, not even on a good version, and does not set the resolution or trust the engineering mode of a version that is not known to be good
  CHECK(!sensorid::command_allowed(Model::kLd2412, v136, sensorid::Risky::kSetBaud) && !sensorid::command_allowed(Model::kLd2450, v136, sensorid::Risky::kSetBaud));
  CHECK(!sensorid::command_allowed(Model::kLd2412, v124, sensorid::Risky::kSetResolution) && !sensorid::command_allowed(Model::kLd2412, unknown, sensorid::Risky::kEngineeringMode));
  CHECK(sensorid::command_allowed(Model::kLd2412, v136, sensorid::Risky::kSetResolution));
  // the gate command is built within the document's limits (LD2412 1.05, 2.2.5: manual example 01 0C 05 00 00)
  std::uint8_t frame[ld2450cmd::kMaxFrame];
  const std::size_t n = ld2450cmd::build_ld2412_base_parameters(1, 13, 5, 0, frame, sizeof frame);
  const std::uint8_t expected[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x07, 0x00, 0x02, 0x00, 0x01, 0x0D, 0x05, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
  CHECK(n == sizeof expected && std::memcmp(frame, expected, n) == 0);
  CHECK(ld2450cmd::build_ld2412_base_parameters(0, 13, 5, 0, frame, sizeof frame) == 0 && ld2450cmd::build_ld2412_base_parameters(5, 3, 5, 0, frame, sizeof frame) == 0 && ld2450cmd::build_ld2412_base_parameters(1, 14, 5, 0, frame, sizeof frame) == 0);
  // reading the mac asks with the value 1
  const std::size_t m = ld2450cmd::build_read_mac(frame, sizeof frame);
  const std::uint8_t mac_expected[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xA5, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};   // manual
  CHECK(m == sizeof mac_expected && std::memcmp(frame, mac_expected, m) == 0);
}

// ---- closing the sensors' own Bluetooth ----------------------------------------------------------------------------------------------------

// A sensor that can be made to misbehave. Its radio state changes when it restarts, as a real one's does.
class FakeSensor : public btguard::Link {
 public:
  bool bluetooth_on = true;          // what is switched on in the sensor's memory (read through the address after a restart)
  bool visible_open = true;          // what the address says now
  bool answers = true;
  bool accepts_write = true;
  bool ignores_write = false;        // acknowledges 0x00A4 but does nothing (the faults of some firmware)
  bool answers_after_restart = true;
  bool in_config = false;
  unsigned writes = 0, restarts = 0, enters = 0, ends = 0, reads = 0;
  unsigned long waited_ms = 0;
  bool enter_config() override { ++enters; if (!answers) return false; in_config = true; return true; }
  bool end_config() override { ++ends; in_config = false; return answers; }
  bool read_mac(sensorid::Mac& out) override {
    ++reads;
    if (!answers || !in_config) return false;
    out = sensorid::Mac{};
    out.known = true;
    out.bluetooth = visible_open ? sensorid::Bluetooth::kOpen : sensorid::Bluetooth::kClosed;
    if (visible_open) { const std::uint8_t b[6] = {0x8F, 0x27, 0x2E, 0xB8, 0x0F, 0x65}; std::memcpy(out.bytes, b, 6); out.text = "8F:27:2E:B8:0F:65"; }
    return true;
  }
  bool set_bluetooth(bool on) override {
    if (!answers || !in_config || !accepts_write) return false;
    ++writes;
    if (!ignores_write) bluetooth_on = on;
    return true;
  }
  bool restart() override {
    if (!answers || !in_config) return false;
    ++restarts;
    in_config = false;
    visible_open = bluetooth_on;
    if (!answers_after_restart) answers = false;
    return true;
  }
  void pause_ms(unsigned ms) override { waited_ms += ms; }
};

static void test_bluetooth_close() {
  using btguard::Outcome;
  {   // open: closed, read back closed, one write and one restart
    FakeSensor s;
    const btguard::Result r = btguard::close(s);
    CHECK(r.outcome == Outcome::kClosed && r.before == sensorid::Bluetooth::kOpen && r.after == sensorid::Bluetooth::kClosed);
    CHECK(s.writes == 1 && s.restarts == 1 && !s.in_config && r.writes == 1 && !s.visible_open && !s.bluetooth_on);
    CHECK(s.waited_ms >= 1000);   // it waited for the sensor to start again
  }
  {   // already closed: nothing is written (the setting lives in the sensor's flash)
    FakeSensor s; s.visible_open = false; s.bluetooth_on = false;
    const btguard::Result r = btguard::close(s);
    CHECK(r.outcome == Outcome::kAlreadyClosed && s.writes == 0 && s.restarts == 0 && !s.in_config);
  }
  {   // the sensor acknowledges and does nothing: still open, said so, not "closed"
    FakeSensor s; s.ignores_write = true;
    const btguard::Result r = btguard::close(s);
    CHECK(r.outcome == Outcome::kStillOpen && r.after == sensorid::Bluetooth::kOpen && s.writes == 1);
  }
  {   // refused
    FakeSensor s; s.accepts_write = false;
    const btguard::Result r = btguard::close(s);
    CHECK(r.outcome == Outcome::kRefused && s.writes == 0 && !s.in_config);
  }
  {   // silent from the start: no state is invented
    FakeSensor s; s.answers = false;
    const btguard::Result r = btguard::close(s);
    CHECK(r.outcome == Outcome::kNoAnswer && r.before == sensorid::Bluetooth::kUnknown && s.writes == 0 && s.enters == 2);   // two tries to enter, no more
  }
  {   // it never comes back after the restart: the state stays unknown, not closed
    FakeSensor s; s.answers_after_restart = false;
    const btguard::Result r = btguard::close(s);
    CHECK(r.outcome == Outcome::kNoAnswer && r.after == sensorid::Bluetooth::kUnknown && s.writes == 1 && s.restarts == 1);
  }
  {   // checking only never writes
    FakeSensor s;
    const btguard::Result r = btguard::check(s);
    CHECK(r.outcome == Outcome::kOpen && s.writes == 0 && s.restarts == 0 && !s.in_config);
    FakeSensor c; c.visible_open = false;
    CHECK(btguard::check(c).outcome == Outcome::kAlreadyClosed);
  }
}

// What the node does with the sensor, minute by minute, for a week: the writes are bounded whatever the sensor does.
struct Week { unsigned writes; unsigned checks; bool alert; bool gave_up; unsigned open_events; };
static Week run_week(FakeSensor& sensor, bool close_after_setup, btguard::Guard::Limits limits = {}) {
  btguard::Guard guard(limits);
  guard.set_model(sensors::Model::kLd2450);
  guard.set_close_after_setup(close_after_setup);
  Week w{0, 0, false, false, 0};
  for (std::uint64_t minute = 0; minute < 7 * 24 * 60; ++minute) {
    const std::uint64_t now = minute * 60000;
    const btguard::Action action = guard.due(now);
    if (action == btguard::Action::kNone) continue;
    const bool closing = action == btguard::Action::kClose;
    const btguard::Result r = closing ? btguard::close(sensor) : btguard::check(sensor);
    w.writes += r.writes;
    if (!closing) ++w.checks;
    guard.record(now, r, closing);
  }
  w.alert = guard.alert();
  w.gave_up = guard.gave_up();
  w.open_events = guard.open_events();
  return w;
}

static void test_bluetooth_guard() {
  {   // the normal case: found open at start, closed, then only quiet read-only looks
    FakeSensor s;
    const Week w = run_week(s, true);
    CHECK(w.writes == 1 && !w.alert && !w.gave_up && w.open_events == 1);
    CHECK(w.checks >= 20 && w.checks <= 40);   // every six hours for a week
  }
  {   // a sensor that ignores the command: two attempts, then it gives up and tells (alert), and never writes again for the rest of the week
    FakeSensor s; s.ignores_write = true;
    const Week w = run_week(s, true);
    CHECK(w.writes == 2 && w.alert && w.gave_up && w.open_events == 1);
  }
  {   // refused every time: bounded as well
    FakeSensor s; s.accepts_write = false;
    const Week w = run_week(s, true);
    CHECK(s.writes == 0 && w.writes == 2 && s.enters <= 2 * 2 + 2 * 30 && w.alert);   // two attempts were made, none was accepted
    CHECK(s.restarts == 0);
  }
  {   // silent sensor: bounded attempts, then a look every six hours; no alert is invented for a state that is not known, but it is not called closed either
    FakeSensor s; s.answers = false;
    const Week w = run_week(s, true);
    CHECK(w.writes == 0 && s.enters < 100);
  }
  {   // the setting is off (the administrator wants the radio): never a write, the state is told, no alert
    FakeSensor s;
    const Week w = run_week(s, false);
    CHECK(w.writes == 0 && !w.alert && w.open_events == 1 && s.bluetooth_on);
  }
  {   // somebody switches it on again later: a new open event, a new bounded round
    FakeSensor s;
    btguard::Guard guard;
    guard.set_model(sensors::Model::kLd2450);
    std::uint64_t now = 10000;
    CHECK(guard.due(now) == btguard::Action::kClose);
    CHECK(guard.record(now, btguard::close(s), true));   // found open the first time
    CHECK(guard.state() == sensorid::Bluetooth::kClosed && !guard.alert());
    s.bluetooth_on = true; s.visible_open = true;        // switched on again by a factory reset
    now += 7ULL * 3600 * 1000;
    CHECK(guard.due(now) == btguard::Action::kCheck);
    CHECK(guard.record(now, btguard::check(s), false));  // a new event
    CHECK(guard.state() == sensorid::Bluetooth::kOpen && guard.alert() && guard.open_events() == 2);
    now += 130000;
    CHECK(guard.due(now) == btguard::Action::kClose);
  }
  {   // a person presses the button after the node gave up: one more bounded round
    FakeSensor s; s.ignores_write = true;
    btguard::Guard guard;
    guard.set_model(sensors::Model::kLd2450);
    std::uint64_t now = 10000;
    for (int i = 0; i < 6; ++i, now += 130000) { if (guard.due(now) == btguard::Action::kClose) guard.record(now, btguard::close(s), true); }
    CHECK(guard.gave_up() && guard.alert() && guard.due(now) == btguard::Action::kNone);
    guard.allow_again();
    CHECK(guard.due(now) == btguard::Action::kClose);
    s.ignores_write = false;
    guard.record(now, btguard::close(s), true);
    CHECK(guard.state() == sensorid::Bluetooth::kClosed && !guard.alert() && !guard.gave_up());
  }
  {   // a model with no radio is left alone
    btguard::Guard guard;
    guard.set_model(sensors::Model::kLd2461);
    CHECK(guard.due(10000000) == btguard::Action::kNone && !guard.alert() && guard.state() == sensorid::Bluetooth::kNone);
  }
  {   // nothing before the first delay
    btguard::Guard guard;
    guard.set_model(sensors::Model::kLd2410);
    CHECK(guard.due(1000) == btguard::Action::kNone && guard.due(9000) == btguard::Action::kClose);
  }
}

// ---- the transparent serial bridge ------------------------------------------------------------------------------------------------------

static void test_bridge() {
  using bridge::Refusal;
  std::uint8_t secret[16];
  for (int i = 0; i < 16; ++i) secret[i] = static_cast<std::uint8_t>(0xA0 + i);
  std::string token;
  bridge::Session session;
  std::uint64_t now = 100000;
  CHECK(!session.active());
  CHECK(session.exchange("x", 0, 0, now) == Refusal::kNoSession);
  CHECK(session.open(4, 256000, 10, "ana", now, secret, token) == Refusal::kBadSensor && session.open(0, 256000, 10, "ana", now, secret, token) == Refusal::kBadSensor);
  CHECK(session.open(1, 12345, 10, "ana", now, secret, token) == Refusal::kBadBaud);
  CHECK(session.open(1, 256000, 0, "ana", now, secret, token) == Refusal::kBadMinutes && session.open(1, 256000, 31, "ana", now, secret, token) == Refusal::kBadMinutes);
  CHECK(!session.active());
  CHECK(session.open(2, 115200, 10, "ana", now, secret, token) == Refusal::kNone && session.active() && session.sensor() == 2 && session.baud() == 115200 && token.size() == 32);
  CHECK(token == "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf");
  CHECK(session.opened_by() == "ana" && session.expires_at_ms() == now + 600000);
  // one sensor at a time
  std::string other;
  CHECK(session.open(3, 115200, 10, "bob", now + 1, secret, other) == Refusal::kBusy && session.sensor() == 2);
  // the secret is checked, sizes and speed are limited
  CHECK(session.exchange("a0a1a2a3a4a5a6a7a8a9aaabacadaeae", 0, 0, now + 10) == Refusal::kBadToken && session.exchange("", 0, 0, now + 10) == Refusal::kBadToken && session.exchange(token + "0", 0, 0, now + 10) == Refusal::kBadToken);
  CHECK(session.exchange(token, 513, 0, now + 10) == Refusal::kTooBig && session.exchange(token, 0, 1025, now + 10) == Refusal::kTooBig);
  CHECK(session.exchange(token, 512, 1024, now + 20) == Refusal::kNone);
  session.note_read(10, 1024);
  CHECK(session.bytes_to_sensor() == 512 && session.bytes_from_sensor() == 10);
  // the line carries about 40 kB a second: past that, wait for the next second
  std::uint64_t t = now + 100;
  Refusal r = Refusal::kNone;
  unsigned accepted = 0;
  for (int i = 0; i < 100 && r == Refusal::kNone; ++i) { r = session.exchange(token, 512, 1024, t); if (r == Refusal::kNone) ++accepted; }
  CHECK(r == Refusal::kTooFast && accepted >= 20 && accepted < 30);
  CHECK(session.exchange(token, 512, 1024, t + 1500) == Refusal::kNone);   // a new second
  // it ends by itself
  CHECK(session.exchange(token, 0, 0, now + 600000) == Refusal::kExpired && !session.active() && session.last_end() == bridge::End::kExpired);
  CHECK(session.exchange(token, 0, 0, now + 600001) == Refusal::kExpired);
  // and another can start; closing by hand
  CHECK(session.open(1, 9600, 1, "bob", now + 700000, secret, token) == Refusal::kNone);
  CHECK(session.close() && !session.close() && session.last_end() == bridge::End::kStopped);
  CHECK(session.exchange(token, 0, 0, now + 700001) == Refusal::kNoSession);
  // a session left open is cleared by the expiry check before a new request
  CHECK(session.open(1, 9600, 1, "bob", now + 800000, secret, token) == Refusal::kNone);
  CHECK(session.open(2, 9600, 1, "bob", now + 800000 + 61000, secret, other) == Refusal::kNone && session.sensor() == 2);
  CHECK(bridge::same_secret("abc", "abc") && !bridge::same_secret("abc", "abd") && !bridge::same_secret("abc", "ab") && bridge::same_secret("", ""));
  CHECK(bridge::baud_allowed(256000) && bridge::baud_allowed(9600) && !bridge::baud_allowed(0) && !bridge::baud_allowed(100));
  CHECK(std::string(bridge::refusal_text(Refusal::kBusy)) == "bridge_busy");
}

// ---- the LD2450 report ------------------------------------------------------------------------------------------------------------------

static Bytes ld2450_frame(const std::vector<std::vector<std::uint8_t>>& targets) {
  Bytes f = {0xAA, 0xFF, 0x03, 0x00};
  for (std::size_t i = 0; i < 3; ++i) {
    if (i < targets.size()) f.insert(f.end(), targets[i].begin(), targets[i].end());
    else f.insert(f.end(), 8, 0);
  }
  f.push_back(0x55); f.push_back(0xCC);
  return f;
}
// A target from its numbers (sign-magnitude: bit 15 set means positive).
static std::vector<std::uint8_t> target(int x, int y, int speed_cm_s, int resolution) {
  const auto word = [](int v) { const unsigned m = static_cast<unsigned>(v < 0 ? -v : v) & 0x7FFF; return static_cast<std::uint16_t>(v < 0 ? m : (m | 0x8000)); };
  const std::uint16_t a = word(x), b = word(y), c = word(speed_cm_s), d = static_cast<std::uint16_t>(resolution);
  return {static_cast<std::uint8_t>(a & 255), static_cast<std::uint8_t>(a >> 8), static_cast<std::uint8_t>(b & 255), static_cast<std::uint8_t>(b >> 8),
          static_cast<std::uint8_t>(c & 255), static_cast<std::uint8_t>(c >> 8), static_cast<std::uint8_t>(d & 255), static_cast<std::uint8_t>(d >> 8)};
}

static void test_parser() {
  ld2450::Frame frame;
  // sign-magnitude: -782 mm sideways, 1713 forward, -16 cm/s, resolution 360 (manual 1.03 sample)
  CHECK(ld2450::decode_frame(ld2450_frame({target(-782, 1713, -16, 360)}).data(), 30, frame));
  CHECK(frame.targets[0].valid && frame.targets[0].x_mm == -782 && frame.targets[0].y_mm == 1713 && frame.targets[0].speed_cm_s == -16 && frame.count() == 1 && frame.valid_count() == 1);
  CHECK(ld2450::decode_signed(0x8001) == 1 && ld2450::decode_signed(0x0001) == -1 && ld2450::decode_signed(0x0000) == 0 && ld2450::decode_signed(0x8000) == 0 && ld2450::decode_signed(0xFFFF) == 32767);
  // a target with no resolution is not a target (the other bytes are leftovers)
  CHECK(ld2450::decode_frame(ld2450_frame({target(500, 2000, 10, 0)}).data(), 30, frame) && frame.count() == 1 && frame.valid_count() == 0 && frame.targets[0].rejected == ld2450::Reject::kNoResolution);
  // beyond what the module measures
  CHECK(ld2450::decode_frame(ld2450_frame({target(6501, 2000, 10, 360)}).data(), 30, frame) && frame.targets[0].rejected == ld2450::Reject::kOutOfRange);
  CHECK(ld2450::decode_frame(ld2450_frame({target(-6501, 2000, 10, 360)}).data(), 30, frame) && frame.targets[0].rejected == ld2450::Reject::kOutOfRange);
  CHECK(ld2450::decode_frame(ld2450_frame({target(0, 8501, 10, 360)}).data(), 30, frame) && frame.targets[0].rejected == ld2450::Reject::kOutOfRange);
  CHECK(ld2450::decode_frame(ld2450_frame({target(0, -501, 10, 360)}).data(), 30, frame) && frame.targets[0].rejected == ld2450::Reject::kOutOfRange);
  CHECK(ld2450::decode_frame(ld2450_frame({target(100, 2000, 1001, 360)}).data(), 30, frame) && frame.targets[0].rejected == ld2450::Reject::kOutOfRange);
  CHECK(ld2450::decode_frame(ld2450_frame({target(100, 2000, -1001, 360)}).data(), 30, frame) && frame.targets[0].rejected == ld2450::Reject::kOutOfRange);
  // the edges are inside
  CHECK(ld2450::decode_frame(ld2450_frame({target(6500, 8500, 1000, 360), target(-6500, -500, -1000, 360)}).data(), 30, frame) && frame.valid_count() == 2);
  // the 2.14 firmware sends an empty slot as a target at the origin with a resolution
  CHECK(ld2450::decode_frame(ld2450_frame({target(0, 0, 0, 360), target(300, 1500, 0, 360)}).data(), 30, frame) && frame.targets[0].rejected == ld2450::Reject::kOrigin && frame.valid_count() == 1);
  // only valid targets become tracks
  Track tracks[3];
  CHECK(ld2450::to_tracks(1, frame, tracks) == 1 && tracks[0].track_id == 2 && tracks[0].x_mm == 300);
  // an empty slot (eight zero bytes) is not even present
  CHECK(ld2450::decode_frame(ld2450_frame({}).data(), 30, frame) && frame.count() == 0 && ld2450::to_tracks(1, frame, tracks) == 0);
  // a wrong header, tail or length is no frame
  Bytes bad = ld2450_frame({target(1, 1, 1, 360)});
  bad[0] = 0xAB;
  CHECK(!ld2450::decode_frame(bad.data(), 30, frame) && frame.count() == 0);
  Bytes tail = ld2450_frame({target(1, 1, 1, 360)});
  tail[29] = 0;
  CHECK(!ld2450::decode_frame(tail.data(), 30, frame) && !ld2450::decode_frame(tail.data(), 29, frame) && !ld2450::decode_frame(nullptr, 30, frame));

  // the stream: garbage before and after, a header inside the data, split in odd pieces
  const Bytes a = ld2450_frame({target(100, 1000, 5, 360)});
  const Bytes inside = ld2450_frame({{0xAA, 0xFF, 0x03, 0x00, 0xAA, 0xFF, 0x03, 0x00}});   // the data of the first target looks like a header
  Bytes stream = {0x00, 0x13, 0xAA, 0xFF, 0x03};   // a header cut short
  stream.insert(stream.end(), a.begin(), a.end());
  stream.insert(stream.end(), {0xFF, 0xFF, 0xFF});   // trailing garbage
  stream.insert(stream.end(), inside.begin(), inside.end());
  stream.insert(stream.end(), a.begin(), a.end());
  for (std::size_t chunk : {1u, 3u, 7u, 30u, 128u}) {
    FrameFramer framer(ld2450::protocol());
    std::vector<Bytes> got;
    for (std::size_t i = 0; i < stream.size(); i += chunk) framer.feed(stream.data() + i, std::min<std::size_t>(chunk, stream.size() - i), [&](const std::uint8_t* f, std::size_t n) { got.emplace_back(f, f + n); });
    CHECK(got.size() == 3 && got[0] == a && got[1] == inside && got[2] == a);
  }
}

// ---- tracking ---------------------------------------------------------------------------------------------------------------------------

static Track det(int x, int y, int speed = 0) { return Track{1, 1, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), static_cast<std::int16_t>(speed)}; }

static void test_tracking() {
  // two people who cross: with the prediction each keeps its identity, with the plain nearest rule they swap
  const auto run = [](bool predict, std::uint8_t& first_id_after, std::uint8_t& second_id_after) {
    StabiliserConfig config;
    config.predict = predict;
    config.smoothing_percent = 100;
    TrackStabiliser s(config);
    s.set_sensor(1);
    Track out[kTracksPerRadar];
    std::uint8_t a_id = 0, b_id = 0;
    for (int frame = 0; frame < 12; ++frame) {
      const int ax = -1125 + 250 * frame, bx = 1125 - 250 * frame;   // they pass through each other between frames 4 and 5
      Track in[2] = {det(ax, 3000, 2500), det(bx, 3000, -2500)};
      in[1].track_id = 2;
      const std::size_t n = s.update(in, 2, 1000 + frame * 100, out);
      for (std::size_t i = 0; i < n; ++i) {
        const bool is_a = std::abs(out[i].x_mm - ax) < std::abs(out[i].x_mm - bx);
        if (frame == 1) { (is_a ? a_id : b_id) = out[i].track_id; }
        if (frame == 11) { if (is_a) first_id_after = out[i].track_id; else second_id_after = out[i].track_id; }
      }
    }
    return std::make_pair(a_id, b_id);
  };
  std::uint8_t a1 = 0, b1 = 0, a2 = 0, b2 = 0;
  const auto before_predict = run(true, a1, b1);
  const auto before_plain = run(false, a2, b2);
  CHECK(before_predict.first != 0 && before_predict.second != 0 && before_predict.first != before_predict.second);
  CHECK(a1 == before_predict.first && b1 == before_predict.second);       // with the prediction: the same people keep their ids
  CHECK(a2 != before_plain.first || b2 != before_plain.second);            // without it they are swapped: the prediction is the better rule

  // one extreme detection does not poison the track: the values are integers in the range of the contract and the sums are 64 bit
  {
    TrackStabiliser s;
    s.set_sensor(1);
    Track out[kTracksPerRadar];
    for (int f = 0; f < 5; ++f) { Track in[1] = {det(1000, 2000)}; s.update(in, 1, 1000 + f * 100, out); }
    Track extreme[1] = {det(32767, 32767, 32767)};
    const std::size_t n = s.update(extreme, 1, 1600, out);
    CHECK(n >= 1);
    Track back[1] = {det(1010, 2010)};
    const std::size_t m = s.update(back, 1, 1700, out);
    CHECK(m >= 1);
    bool near_home = false;
    for (std::size_t i = 0; i < m; ++i) near_home = near_home || (std::abs(out[i].x_mm - 1000) < 200 && std::abs(out[i].y_mm - 2000) < 200);
    CHECK(near_home);   // the track is where it was, not dragged away
    // opposite corners of the 16 bit plane in one step: a squared distance of 8.6e9 does not fit 32 bits
    TrackStabiliser t;
    t.set_sensor(1);
    Track corner[1] = {det(-32768, -32768)};
    t.update(corner, 1, 1000, out);
    Track opposite[1] = {det(32767, 32767)};
    const std::size_t k = t.update(opposite, 1, 1100, out);
    CHECK(k >= 1);
  }
  // a calibration that would push a position past the 16 bit range saturates, it does not wrap
  {
    config::RadarLine line;
    line.offset_x_mm = 5000; line.yaw_deg = 90;
    Track far{1, 1, 32000, 32000, 0};
    const Track moved = calibration::apply(far, line);
    CHECK(moved.x_mm == -32000 + 5000 || moved.x_mm >= -32768);
  }
}

static void test_pet() {
  presence::Reading r;
  r.valid = true; r.present = true; r.moving = true; r.state = 1; r.moving_cm = 120; r.moving_energy = 12; r.distance_cm = 120;
  pet::Rule rule;
  CHECK(!pet::looks_like_pet(r, rule));            // off by default
  rule.enabled = true;
  CHECK(pet::looks_like_pet(r, rule));             // near and weak
  r.moving_energy = 60;
  CHECK(!pet::looks_like_pet(r, rule));            // strong: somebody
  r.moving_energy = 12; r.distance_cm = 300;
  CHECK(!pet::looks_like_pet(r, rule));            // far: not a pet by this rule
  r.distance_cm = 120; r.moving_energy = -1;
  CHECK(!pet::looks_like_pet(r, rule));            // energy not known: never taken for a pet
  r.moving_energy = 12; r.stationary = true; r.static_energy = 80; r.state = 3;
  CHECK(!pet::looks_like_pet(r, rule));            // a strong stationary target too
  r.static_energy = 10;
  CHECK(pet::looks_like_pet(r, rule));
  bool was = false;
  const presence::Reading out = pet::filtered(r, rule, was);
  CHECK(was && !out.present && out.distance_cm == -1 && out.state == 0);
  presence::Reading none;
  CHECK(!pet::looks_like_pet(none, rule));
}

static void test_mr24_and_report() {
  // the firmware question (manual V1.5): 53 59 02 A4 00 01 0F, the sum, 54 43
  std::uint8_t frame[16];
  const std::size_t n = presence::mr24_build_firmware_query(frame, sizeof frame);
  const std::uint8_t expected[] = {0x53, 0x59, 0x02, 0xA4, 0x00, 0x01, 0x0F, static_cast<std::uint8_t>(0x53 + 0x59 + 0x02 + 0xA4 + 0x00 + 0x01 + 0x0F), 0x54, 0x43};
  CHECK(n == sizeof expected && std::memcmp(frame, expected, n) == 0 && presence::mr24_build_firmware_query(frame, 5) == 0);
  // the answer: text in the data, with a checksum; anything that is not text is not shown
  const auto answer = [](const std::string& text, std::uint8_t first_byte = 0) {
    Bytes f = {0x53, 0x59, 0x02, 0xA4, 0x00, static_cast<std::uint8_t>(text.size() + (first_byte ? 1 : 0))};
    if (first_byte) f.push_back(first_byte);
    f.insert(f.end(), text.begin(), text.end());
    std::uint8_t sum = 0;
    for (std::uint8_t b : f) sum = static_cast<std::uint8_t>(sum + b);
    f.push_back(sum); f.push_back(0x54); f.push_back(0x43);
    return f;
  };
  presence::Mr24Event event;
  Bytes ok = answer("HPC1V1.5.2");
  CHECK(presence::decode_mr24(ok.data(), ok.size(), event) && event.kind == presence::Mr24Kind::kFirmware && std::string(event.text) == "HPC1V1.5.2");
  presence::Mr24State state;
  CHECK(!state.apply(event) && state.firmware == "HPC1V1.5.2");
  Bytes binary = answer("abc", 0x07);
  CHECK(presence::decode_mr24(binary.data(), binary.size(), event) && event.kind == presence::Mr24Kind::kNone);   // a binary version: nothing is invented
  Bytes wrong = answer("HPC1V1.5.2");
  wrong[wrong.size() - 3] ^= 1;
  CHECK(!presence::decode_mr24(wrong.data(), wrong.size(), event));
  // a report of the occupancy is still read as before
  const Bytes occupied = {0x53, 0x59, 0x80, 0x01, 0x00, 0x01, 0x01, static_cast<std::uint8_t>(0x53 + 0x59 + 0x80 + 0x01 + 0x01 + 0x01), 0x54, 0x43};
  CHECK(presence::decode_mr24(occupied.data(), occupied.size(), event) && event.kind == presence::Mr24Kind::kOccupied && event.value == 1);

  // the entries of the health report
  radarreport::Sensor a;
  a.sensor_id = 1; a.model = "ld2450"; a.state = "reporting"; a.firmware_known = true; a.firmware = "1.02.22062416"; a.bluetooth = sensorid::Bluetooth::kOpen; a.mac = "8F:27:2E:B8:0F:65"; a.bluetooth_alert = true;
  radarreport::Sensor b;
  b.sensor_id = 2; b.model = "ld2412"; b.state = "silent"; b.bluetooth = sensorid::Bluetooth::kUnknown; b.quirk = "ld2412_unknown_firmware";
  radarreport::Sensor c;
  c.sensor_id = 3; c.model = "ld2461"; c.state = "bridge"; c.bluetooth = sensorid::Bluetooth::kNone; c.firmware_known = true; c.firmware = "bad text!";   // not letters and digits: left out
  radarreport::Radar radar;
  radar.perimeter_enabled = true; radar.events_buffered = 3;
  const std::string members = radarreport::members_json({a, b, c}, radar);
  CHECK(members == "\"sensors\":[{\"sensor_id\":1,\"model\":\"ld2450\",\"state\":\"reporting\",\"firmware_known\":true,\"firmware\":\"1.02.22062416\",\"bluetooth\":\"open\",\"mac\":\"8F:27:2E:B8:0F:65\",\"bluetooth_alert\":true},"
                      "{\"sensor_id\":2,\"model\":\"ld2412\",\"state\":\"silent\",\"firmware_known\":false,\"bluetooth\":\"unknown\",\"quirk\":\"ld2412_unknown_firmware\"},"
                      "{\"sensor_id\":3,\"model\":\"ld2461\",\"state\":\"bridge\",\"firmware_known\":true,\"bluetooth\":\"none\"}],"
                      "\"radar\":{\"perimeter_enabled\":true,\"masked\":false,\"wifi_jam\":false,\"bridge_open\":false,\"events_buffered\":3,\"sensor_bluetooth_off\":true}");
  CHECK(radarreport::splice("{\"kind\":\"node_health\"}", members) == "{\"kind\":\"node_health\"," + members + "}");
  CHECK(radarreport::splice("{}", "\"a\":1") == "{\"a\":1}" && radarreport::splice("not json", "\"a\":1") == "not json" && radarreport::splice("{\"a\":1}", "") == "{\"a\":1}");
  radarreport::Sensor bad;
  bad.sensor_id = 4;
  CHECK(radarreport::members_json({bad}, radar).find("\"sensors\":[]") == 0);
  CHECK(std::string(radarreport::quirk_of(sensors::Model::kLd2412, sensorid::Firmware{})) == "ld2412_unknown_firmware" && std::string(radarreport::quirk_of(sensors::Model::kLd2450, sensorid::Firmware{})).empty());
  CHECK(radarreport::safe_text("1.02.22062416", 24) && !radarreport::safe_text("", 24) && !radarreport::safe_text("a b", 24) && !radarreport::safe_text(std::string(25, 'a'), 24));
}

static void test_bluetooth_suspend() {
  FakeSensor s;
  btguard::Guard guard;
  guard.set_model(sensors::Model::kLd2450);
  std::uint64_t now = 10000;
  guard.record(now, btguard::close(s), true);
  CHECK(guard.state() == sensorid::Bluetooth::kClosed);
  // a person opens the radio to use the phone app: the rule waits half an hour, then closes it again
  s.bluetooth_on = true; s.visible_open = true;
  guard.suspend_until(now + 30ULL * 60 * 1000);
  CHECK(guard.state() == sensorid::Bluetooth::kUnknown && !guard.alert() && guard.due(now + 60000) == btguard::Action::kNone && guard.due(now + 29ULL * 60 * 1000) == btguard::Action::kNone);
  CHECK(guard.due(now + 30ULL * 60 * 1000) == btguard::Action::kClose);
  guard.record(now + 30ULL * 60 * 1000, btguard::close(s), true);
  CHECK(guard.state() == sensorid::Bluetooth::kClosed && !s.visible_open);
  CHECK(guard.due(now + 31ULL * 60 * 1000) == btguard::Action::kNone);
}

int main() {
  test_identity();
  test_ld2412_quirks();
  test_bluetooth_close();
  test_bluetooth_guard();
  test_bridge();
  test_parser();
  test_tracking();
  test_pet();
  test_mr24_and_report();
  test_bluetooth_suspend();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
