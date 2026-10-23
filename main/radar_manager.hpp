// ARMOR-NODE-RADAR - the sensors on the three serial ports: trackers (LD2450, LD2461) and presence sensors (LD2410, LD2412, LD2410S, MR24HPC1).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "core/ld2450_command.hpp"
#include "core/node_config.hpp"
#include "core/pet_filter.hpp"
#include "core/radar_event.hpp"
#include "core/radar_health.hpp"
#include "core/radar_report.hpp"
#include "radar_tracks.hpp"

namespace armor::radar {

// Sends a message to the broker (the manager knows nothing about MQTT): a presence sensor is published as a device of the server.
using Publisher = std::function<void(const std::string& topic, const std::string& payload)>;

// Opens the serial port of every enabled sensor on its pins, at the speed of its model (or the one set), and starts the task that reads them.
// A tracker feeds the tracks of the perimeter; a presence sensor is published through `publisher` as armor/device/<node>/<name>/state.
void start(const config::Settings& settings, Publisher publisher);

// The broker (re)connected: every presence sensor publishes its state again at once.
void publish_presence_now();

// True while at least one tracker has reported a frame recently; the newest tracks of every tracker.
bool any_fresh();
std::size_t collect_tracks(Track* out);

struct Status {
  bool enabled = false;
  int rx = -1, tx = -1;
  std::string model;                // ld2450, ld2461, ld2410, ...
  bool tracker = true;
  std::string name;                 // a presence sensor's device name
  const char* state = "disabled";   // reporting, no data, garbled, silent, bridge, disabled
  std::string state_text;
  std::uint32_t bytes = 0, frames = 0, bad_frames = 0;
  double frames_per_second = 0.0;   // over the last few seconds
  std::string firmware;             // from the last read_info, "" when never read or when the sensor does not know it
  bool firmware_known = false;
  std::string serial;               // LD2410S
  std::string mac;                  // the sensor's own Bluetooth address, while its radio is open
  std::string bluetooth = "unknown";   // open, closed, unknown (not read yet) or none (the model has no radio of its own)
  bool bluetooth_alert = false;     // open (or could not be closed) while the rule is to keep it closed
  bool bluetooth_found_open = false;   // it was found open at some time since the node started
  unsigned bluetooth_attempts = 0;
  bool bluetooth_gave_up = false;
  std::string quirk;                // "", ld2412_bad_firmware or ld2412_unknown_firmware
  std::string gates;                // LD2412 with gates set in the settings: "", applied, verified, reapplied or mismatch
  std::uint32_t pet_ignored = 0;    // readings the pet immunity turned into "nobody"
  bool bridge = false;              // the serial bridge has the sensor: it is not read
  int tracking_mode = 0;            // LD2450: 1 = one target, 2 = up to three, 0 = unknown; LD2461: its report format (1, 2 or 3)
  int present = -1;                 // a presence sensor: 1 somebody, 0 nobody, -1 not known yet
  int distance_cm = -1;             // a presence sensor: how far, when it says
  std::string detail;               // JSON of the latest reading (a presence sensor's numbers, an LD2461's zones), "" when there is none
};
Status status(std::size_t index);

enum class Op { kReadInfo, kSingleTarget, kMultiTarget, kSetZones, kRestart, kFactoryReset, kBluetooth, kSecureBluetooth };

struct CommandResult {
  bool ok = false;
  std::string error;                // "no_tx", "disabled", "busy", "no_answer", "refused", "bad_argument", "unsupported", "bridge_active", "still_open"
  std::string firmware;
  bool firmware_known = false;
  std::string mac;
  std::string bluetooth;            // open, closed or unknown, after the command
  std::string serial;
  int tracking_mode = 0;
  bool has_zones = false;
  ld2450cmd::ZoneFilter zones;
  std::string last_answer_hex;      // the module's last raw acknowledgement, shown in the panel so a mismatch is seen at once
};

// Runs one configuration command on a sensor, the way its model does it (enter configuration, the command, leave configuration; the LD2461 has no
// configuration mode). Blocks for up to a few seconds. `flag` is the bluetooth on/off; `zones` is required for kSetZones. A command the model does not
// have is answered with the error "unsupported". kSecureBluetooth closes the sensor's own radio (see core/sensor_bluetooth.hpp) and reads it back.
CommandResult run(std::size_t index, Op op, bool flag = false, const ld2450cmd::ZoneFilter* zones = nullptr);

// ---- what the perimeter engine and the settings tell the manager -----------------------------------------------------------------------------------

// The pet immunity rule (core/pet_filter.hpp) and the rule for the sensors' own Bluetooth: both apply at once.
void set_pet_rule(const pet::Rule& rule);
void set_close_bluetooth(bool close_after_setup);
bool close_bluetooth_rule();

// Where the events of the sensors go (the Bluetooth of a sensor found open or closed): the perimeter module delivers them.
using EventSink = std::function<void(int sensor_id, radarevent::Kind kind, const std::string& name)>;
void set_event_sink(EventSink sink);

// One entry per enabled sensor for the health report (see core/radar_report.hpp).
std::vector<radarreport::Sensor> report_sensors();

// ---- the serial bridge (see sensor_bridge.hpp) -------------------------------------------------------------------------------------------------

// Hands the sensor's serial port to the bridge: its reading is paused, the port is set to `baud`, and commands to it are refused until bridge_end(). False when the sensor is not
// enabled, its TX wire is not connected (the tool could not write) or a command is running.
bool bridge_begin(std::size_t index, std::uint32_t baud);
// Writes `write_length` bytes to the sensor and returns what it sent back: waits up to `wait_ms` for the first byte, then takes what has arrived (at most `capacity`).
std::size_t bridge_exchange(std::size_t index, const std::uint8_t* write, std::size_t write_length, std::uint8_t* out, std::size_t capacity, unsigned wait_ms);
// Gives the port back: the speed the sensor is read at, an empty buffer, and the reading starts again.
void bridge_end(std::size_t index);

}  // namespace armor::radar
