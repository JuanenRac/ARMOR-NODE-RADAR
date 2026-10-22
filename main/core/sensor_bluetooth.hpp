// ARMOR-RADAR - closing the Bluetooth of the sensors themselves. Pure: tested on a computer with a fake sensor.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// An LD2450, LD2410 or LD2412 has a Bluetooth radio of its own that is ON from the factory and asks for no PIN and no pairing: anybody near the node with the manufacturer's phone app can
// read the sensor and change its filters. The manual gives the way out: in configuration mode send 0x00A4 with the value 0x0000, restart the sensor with 0x00A3, and read the state back with
// 0x00A5 (the address it answers is 08:05:04:03:02:01 while the radio is off).
//
// This file is the procedure and the rule for when to try. It never loops: a sensor that is found open is closed with a bounded number of attempts spaced apart, then left alone and
// reported; only a person starting the action again (the panel's button) allows more. The sensor is reached through `Link`, which the firmware implements over the serial port and the
// tests implement with a sensor that can be made to misbehave.
#pragma once
#include <cstdint>

#include "sensor_identity.hpp"

namespace armor::btguard {

class Link {
 public:
  virtual ~Link() = default;
  virtual bool enter_config() = 0;                 // 0x00FF; false when the sensor did not answer
  virtual bool end_config() = 0;                   // 0x00FE
  virtual bool read_mac(sensorid::Mac& out) = 0;   // 0x00A5 with value 1; false when it did not answer (a refusal included)
  virtual bool set_bluetooth(bool on) = 0;         // 0x00A4; false when it was not accepted
  virtual bool restart() = 0;                      // 0x00A3; the sensor restarts after answering, it is not in configuration mode afterwards
  virtual void pause_ms(unsigned ms) = 0;
};

enum class Outcome : std::uint8_t {
  kNotChecked,
  kAlreadyClosed,   // it was closed already, nothing was written
  kClosed,          // it was open, it was closed and read back closed
  kStillOpen,       // it was written and the sensor still answers with a real address (or never answered again)
  kRefused,         // the sensor refused the command
  kNoAnswer,        // the sensor did not answer, so its state is not known
  kOpen,            // check only: it is open
  kUnsupported,     // the model has no Bluetooth of its own
};

struct Result {
  Outcome outcome = Outcome::kNotChecked;
  sensorid::Bluetooth before = sensorid::Bluetooth::kUnknown;
  sensorid::Bluetooth after = sensorid::Bluetooth::kUnknown;
  sensorid::Mac mac;           // the last address read
  unsigned writes = 0;         // how many times a command that changes the sensor was sent
};

namespace detail {
inline bool enter(Link& link) {
  if (link.enter_config()) return true;
  link.pause_ms(200);
  return link.enter_config();
}
}  // namespace detail

// Only reads: is the radio open? Leaves the sensor as it found it.
inline Result check(Link& link) {
  Result r;
  if (!detail::enter(link)) { r.outcome = Outcome::kNoAnswer; return r; }
  sensorid::Mac mac;
  const bool read = link.read_mac(mac);
  link.end_config();
  if (!read) { r.outcome = Outcome::kNoAnswer; return r; }
  r.mac = mac;
  r.before = r.after = mac.bluetooth;
  r.outcome = mac.bluetooth == sensorid::Bluetooth::kClosed ? Outcome::kAlreadyClosed : mac.bluetooth == sensorid::Bluetooth::kOpen ? Outcome::kOpen : Outcome::kNoAnswer;
  return r;
}

// Closes it: enter configuration, read the address, write 0x0000, restart, wait, enter again, read it back. At most one write and one restart per call. A sensor that is already closed is
// not written to (the setting survives power loss in the sensor's flash; writing it again would only wear it).
inline Result close(Link& link, unsigned restart_wait_ms = 2000) {
  Result r;
  if (!detail::enter(link)) { r.outcome = Outcome::kNoAnswer; return r; }
  sensorid::Mac before;
  if (!link.read_mac(before)) { link.end_config(); r.outcome = Outcome::kNoAnswer; return r; }
  r.mac = before;
  r.before = before.bluetooth;
  if (before.bluetooth == sensorid::Bluetooth::kClosed) { link.end_config(); r.after = before.bluetooth; r.outcome = Outcome::kAlreadyClosed; return r; }
  ++r.writes;
  if (!link.set_bluetooth(false)) { link.end_config(); r.outcome = Outcome::kRefused; return r; }
  if (!link.restart()) { link.end_config(); r.outcome = Outcome::kNoAnswer; return r; }   // the setting is written; it takes effect at the next start
  link.pause_ms(restart_wait_ms);
  if (!detail::enter(link)) { r.outcome = Outcome::kNoAnswer; return r; }
  sensorid::Mac after;
  const bool read = link.read_mac(after);
  link.end_config();
  if (!read) { r.outcome = Outcome::kNoAnswer; return r; }
  r.mac = after;
  r.after = after.bluetooth;
  r.outcome = after.bluetooth == sensorid::Bluetooth::kClosed ? Outcome::kClosed : Outcome::kStillOpen;
  return r;
}

// ---- when to try --------------------------------------------------------------------------------------------------------------------

enum class Action : std::uint8_t { kNone, kCheck, kClose };

// What the node believes about one sensor's radio, and the rule that decides what to do next. `close_after_setup` is the setting "disable the sensors' Bluetooth after set-up" (on by default).
class Guard {
 public:
  struct Limits {
    unsigned max_attempts = 2;                    // writes per open event, without a person asking
    std::uint32_t first_after_ms = 8000;          // after the node started: the sensors need a moment
    std::uint32_t retry_gap_ms = 120000;          // between two attempts
    std::uint32_t recheck_ms = 6u * 3600u * 1000u;   // a read-only look, to learn that somebody switched it on again
  };

  Guard() = default;
  explicit Guard(const Limits& limits) : limits_(limits) {}

  void set_model(sensors::Model model) { has_bluetooth_ = sensorid::has_own_bluetooth(model); state_ = sensorid::initial_bluetooth(model); }
  void set_close_after_setup(bool on) { close_after_setup_ = on; }
  bool close_after_setup() const { return close_after_setup_; }

  sensorid::Bluetooth state() const { return state_; }
  bool found_open() const { return found_open_; }        // it was open at some time since the node started: the security event has been (or will be) told
  bool gave_up() const { return gave_up_; }
  unsigned attempts() const { return attempts_; }
  unsigned open_events() const { return open_events_; }
  // The security alert: the radio is open (or could not be closed or read) while the rule is to keep it closed.
  bool alert() const { return has_bluetooth_ && close_after_setup_ && (state_ == sensorid::Bluetooth::kOpen || gave_up_); }

  // What to do now. Never more than one thing; kNone while the sensor is being left alone.
  Action due(std::uint64_t now_ms) const {
    if (!has_bluetooth_ || now_ms < suspended_until_ms_) return Action::kNone;
    const Action wanted = close_after_setup_ ? Action::kClose : Action::kCheck;
    if (pending_) return wanted;   // the wait that a person asked for is over
    if (!started_) return now_ms >= limits_.first_after_ms ? wanted : Action::kNone;
    const std::uint64_t elapsed = now_ms - last_try_ms_;
    const bool trying = (state_ == sensorid::Bluetooth::kOpen && close_after_setup_) || state_ == sensorid::Bluetooth::kUnknown;
    if (trying && !gave_up_ && attempts_ < limits_.max_attempts && elapsed >= limits_.retry_gap_ms) return wanted;
    if (elapsed >= limits_.recheck_ms) return Action::kCheck;   // a read-only look, however it went before: bounded by the hours, not by attempts
    return Action::kNone;
  }

  // Takes the result of an attempt. Returns true when the sensor was found open for the first time since it was last known closed (the moment to tell History).
  bool record(std::uint64_t now_ms, const Result& result, bool was_close) {
    started_ = true;
    pending_ = false;
    last_try_ms_ = now_ms;
    const sensorid::Bluetooth was = state_;
    const bool opened = result.before == sensorid::Bluetooth::kOpen || result.outcome == Outcome::kOpen;
    bool new_open = false;
    if (opened) {
      new_open = !found_open_ || was == sensorid::Bluetooth::kClosed;
      if (was == sensorid::Bluetooth::kClosed) { attempts_ = 0; gave_up_ = false; }   // somebody switched it on again: a new event, a new bounded round
      found_open_ = true;
      if (new_open) ++open_events_;
    }
    if (was_close || was == sensorid::Bluetooth::kUnknown) ++attempts_;
    switch (result.outcome) {
      case Outcome::kAlreadyClosed: case Outcome::kClosed: state_ = sensorid::Bluetooth::kClosed; gave_up_ = false; attempts_ = 0; break;
      case Outcome::kOpen: case Outcome::kStillOpen: state_ = sensorid::Bluetooth::kOpen; break;
      case Outcome::kRefused: case Outcome::kNoAnswer: case Outcome::kNotChecked: case Outcome::kUnsupported: break;   // a lost answer does not make an open radio look closed
    }
    // what the attempt read before it wrote is what is known, even when the write was refused or the sensor then went silent
    if (result.before != sensorid::Bluetooth::kUnknown && result.outcome != Outcome::kClosed && result.outcome != Outcome::kAlreadyClosed) state_ = result.before;
    if (attempts_ >= limits_.max_attempts && state_ != sensorid::Bluetooth::kClosed) gave_up_ = true;
    return new_open;
  }

  // A person opened the radio on purpose (to use the maker's phone app): the rule waits until `until_ms`, then closes it again like any other time it finds it open.
  void suspend_until(std::uint64_t until_ms) { suspended_until_ms_ = until_ms; pending_ = true; state_ = has_bluetooth_ ? sensorid::Bluetooth::kUnknown : state_; attempts_ = 0; gave_up_ = false; }
  bool suspended(std::uint64_t now_ms) const { return now_ms < suspended_until_ms_; }

  // A person pressed the button: another bounded round is allowed.
  void allow_again() { gave_up_ = false; attempts_ = 0; last_try_ms_ = 0; }

 private:
  Limits limits_;
  bool has_bluetooth_ = true;
  bool close_after_setup_ = true;
  sensorid::Bluetooth state_ = sensorid::Bluetooth::kUnknown;
  bool started_ = false;
  bool found_open_ = false;
  bool gave_up_ = false;
  unsigned attempts_ = 0;
  unsigned open_events_ = 0;
  std::uint64_t last_try_ms_ = 0;
  std::uint64_t suspended_until_ms_ = 0;
  bool pending_ = false;
};

}  // namespace armor::btguard
