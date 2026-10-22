// ARMOR-RADAR - the transparent serial bridge to one sensor, for the maker's PC tools. The rules are in core/serial_bridge.hpp (tested on a computer); this file joins them to the serial port.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// An administrator opens a session for one sensor (the panel, or tools/serial_bridge_client.py on the PC where the maker's tool runs); the node stops reading that sensor, and every exchange of
// the tool - the bytes it writes and the bytes the sensor answers - goes through the node's HTTPS panel with the administrator's login and the session's own secret. The session ends by itself.
// The sensors are powered with 5 V by the node, with no switch: a tool that needs the sensor to start in its bootloader, or to open its Bluetooth window, needs the supply of the sensor cut by hand.
#pragma once
#include <cstdint>
#include <string>

namespace armor::sensor_bridge {

// Starts the task that closes a session whose time is up even when nobody calls (the sensor is then read again).
void start();

struct Opened {
  bool ok = false;
  std::string error;        // bridge_busy, bridge_bad_sensor, bridge_bad_baud, bridge_bad_minutes or bridge_unavailable
  std::string token;
  int sensor = 0;
  std::uint32_t baud = 0;
  unsigned minutes = 0;
};
// `sensor` 1 to 3; `baud` 0: the sensor's own speed. Only for an administrator (the caller checks); `user` is written in the log and in the event.
Opened open(std::size_t sensor, std::uint32_t baud, unsigned minutes, const std::string& user);

struct Exchanged {
  bool ok = false;
  std::string error;        // bridge_no_session, bridge_bad_token, bridge_expired, bridge_too_big or bridge_too_fast
  std::string bytes;        // what the sensor sent
};
Exchanged io(const std::string& token, const std::uint8_t* write, std::size_t write_length, std::size_t want_read, unsigned wait_ms);

bool close(const std::string& user);

struct Status {
  bool active = false;
  int sensor = 0;
  std::uint32_t baud = 0;
  unsigned seconds_left = 0;
  std::string by;
  std::uint64_t to_sensor = 0, from_sensor = 0;
};
Status status();

}  // namespace armor::sensor_bridge
