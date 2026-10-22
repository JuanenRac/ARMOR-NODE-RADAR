// ARMOR-RADAR - what the radar node adds to its health report: one entry per sensor (who it is, its firmware, its Bluetooth) and a few facts about the perimeter engine. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The health report every node says about itself (core/node_health.hpp, shared with the other nodes) is left as it is; the radar node adds two optional members to it before it is sent,
// `sensors` and `radar` (contract node_health.schema.json in ARMOR-COMMON). The words are fixed lists, and the only text taken from a sensor is its firmware version and its Bluetooth address, both cut
// to a few characters of letters and digits.
#pragma once
#include <string>
#include <vector>

#include "json.hpp"
#include "sensor_identity.hpp"
#include "sensor_model.hpp"

namespace armor::radarreport {

struct Sensor {
  int sensor_id = 0;                 // 1 to 3
  std::string model;                 // ld2450, ld2461, ld2410, ld2412, ld2410s, mr24hpc1
  std::string state;                 // reporting, no_data, garbled, silent, bridge
  bool firmware_known = false;
  std::string firmware;              // only when known
  sensorid::Bluetooth bluetooth = sensorid::Bluetooth::kUnknown;
  std::string mac;                   // the sensor's Bluetooth address while its radio is open
  bool bluetooth_alert = false;      // open (or not closable) while the rule is to keep it closed
  std::string quirk;                 // "", ld2412_bad_firmware or ld2412_unknown_firmware
};

struct Radar {
  bool perimeter_enabled = false;
  bool masked = false;
  bool wifi_jam = false;
  bool bridge_open = false;
  unsigned events_buffered = 0;
  bool sensor_bluetooth_off = true;  // the rule (setting)
};

inline bool safe_text(const std::string& text, std::size_t longest) {
  if (text.empty() || text.size() > longest) return false;
  for (char c : text) if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '.' || c == ':' || c == '-' || c == '_')) return false;
  return true;
}

inline const char* quirk_of(sensors::Model model, const sensorid::Firmware& firmware) {
  switch (sensorid::ld2412_level(model, firmware)) {
    case sensorid::Ld2412Level::kBad: return "ld2412_bad_firmware";
    case sensorid::Ld2412Level::kUnknown: return "ld2412_unknown_firmware";
    default: return "";
  }
}

// The two members, without the braces around them: `"sensors":[...],"radar":{...}`.
inline std::string members_json(const std::vector<Sensor>& sensors, const Radar& radar) {
  json::Writer w;
  w.begin_object();   // the writer needs an object to put the keys in; the braces are cut off below
  w.key("sensors").begin_array();
  for (const Sensor& s : sensors) {
    if (s.sensor_id < 1 || s.sensor_id > 3) continue;
    w.begin_object().field("sensor_id", s.sensor_id).field("model", s.model).field("state", s.state);
    w.field("firmware_known", s.firmware_known);
    if (s.firmware_known && safe_text(s.firmware, 24)) w.field("firmware", s.firmware);
    w.field("bluetooth", sensorid::bluetooth_text(s.bluetooth));
    if (s.bluetooth == sensorid::Bluetooth::kOpen && safe_text(s.mac, 17)) w.field("mac", s.mac);
    if (s.bluetooth_alert) w.field("bluetooth_alert", true);
    if (!s.quirk.empty()) w.field("quirk", s.quirk);
    w.end_object();
  }
  w.end_array();
  w.key("radar").begin_object().field("perimeter_enabled", radar.perimeter_enabled).field("masked", radar.masked).field("wifi_jam", radar.wifi_jam).field("bridge_open", radar.bridge_open)
      .field("events_buffered", radar.events_buffered > 1000 ? 1000u : radar.events_buffered).field("sensor_bluetooth_off", radar.sensor_bluetooth_off).end_object();
  w.end_object();
  const std::string whole = w.str();
  return whole.substr(1, whole.size() - 2);
}

// Puts members at the end of a JSON object (the shared health report) without touching the rest of it. A text that is not an object is returned as it is.
inline std::string splice(const std::string& object, const std::string& members) {
  if (members.empty() || object.size() < 2 || object.front() != '{' || object.back() != '}') return object;
  const bool empty = object.size() == 2;
  return object.substr(0, object.size() - 1) + (empty ? "" : ",") + members + "}";
}

}  // namespace armor::radarreport
