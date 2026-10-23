// ARMOR-NODE-RADAR - the settings of a node: what the web panel edits, what is stored in flash and what the firmware obeys.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// One document (JSON) holds everything that differs from node to node, so the same firmware image serves every node and nothing
// secret has to be compiled in. This file only reads, checks and writes that document; it touches no hardware, so all of it is tested
// on a computer. Passwords are never written back to the panel: a section sent without a password (or with an empty one) keeps the
// stored one, and "password_set" tells the panel that there is one.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "board_pins.hpp"
#include "admin_sources.hpp"
#include "json.hpp"
#include "mqtt_tls.hpp"
#include "net_text.hpp"
#include "node_id.hpp"
#include "perimeter_config.hpp"
#include "stored_load.hpp"
#include "uart_wake.hpp"
#include "sensor_model.hpp"

namespace armor::config {

constexpr int kVersion = 1;
constexpr std::size_t kRadarCount = 3;
constexpr std::size_t kMaxMappedPins = 16;
constexpr std::size_t kMaxPasswordText = 64;

enum class Uplink { kEthernet, kWifi };
enum class WifiSecurity { kOpen, kWpa2, kWpa3, kWpa2Wpa3 };
enum class PinMode { kDisabled, kInput, kOutput, kPwm, kAdc };
enum class Pull { kNone, kUp, kDown };
enum class SafeState { kOff, kOn, kKeep };
// When the node accepts configuration over Bluetooth: never, only while it has no user (set-up), or always.
enum class BleMode { kOff, kSetup, kAlways };
// The panel over plain HTTP only, over HTTP and HTTPS (a certificate the node made for itself), or over HTTPS only (port 80 sends the browser to HTTPS).
enum class WebMode { kHttp, kBoth, kHttps };

struct IpSettings {
  bool dhcp = true;
  std::string address, netmask = "255.255.255.0", gateway, dns1, dns2;
  std::string hostname;  // empty: "armor-" + the node id
};

struct AccessPoint {
  bool enabled = false;
  std::string ssid = "ARMOR";
  WifiSecurity security = WifiSecurity::kWpa2;
  std::string password;
  int channel = 0;  // 0: one of 1, 6 or 11 chosen from the node's MAC, so neighbouring nodes rarely share a channel
  bool hidden = false;
  int max_clients = 8;
  int tx_power_dbm = 15;
  int bandwidth_mhz = 20;
  std::string country = "ES";  // two capital letters: sets which channels and how much power the radio may use
  bool bridge = true;  // the access point joins the wired network (one network, one address range); false: its own network
};

struct Network { std::string ssid, password; };
constexpr std::size_t kMaxBackupNetworks = 3;

struct Station {
  bool enabled = false;
  std::string ssid, password;
  // Tried in order, after the network above, whenever the current one cannot be joined for a while (main/network.cpp); never while the
  // network above still works. The same Wi-Fi password rules apply to each.
  std::vector<Network> backup;
};

struct Broker { std::string uri, username, password; };
constexpr std::size_t kMaxBackupBrokers = 2;

struct Mqtt {
  bool enabled = false;  // a node that was never configured has no broker yet
  std::string uri, username, password;
  // Off by default: plain MQTT, as always. On: mqtts:// with the broker's certificate checked against ca_pem ONLY (the installation's own authority, PEM, see mqtt_tls.hpp).
  bool tls = false;
  std::string ca_pem;
  int heartbeat_s = 10;
  int telemetry_ms = 200;
  // Tried in order, after the broker above, whenever it cannot be reached for a while (main/mqtt_link.cpp); never while it still works.
  std::vector<Broker> backup;
};

// A rectangle of the radar's own plane between two opposite corners, in millimetres.
struct ZoneRect { int x1 = 0, y1 = 0, x2 = 0, y2 = 0; };
constexpr std::size_t kZonesPerRadar = 3;

struct RadarLine {
  bool enabled = false;
  std::string model = "ld2450";   // which sensor is on this serial port (core/sensor_model.hpp)
  int baud = 0;                    // 0: the model's own speed
  std::string name;                // a presence sensor is a device of the server with this name (armor/device/<node>/<name>/state); trackers do not use it
  int rx = -1;  // the GPIO that receives the radar's TX line
  int tx = -1;  // the GPIO that sends to the radar's RX line; -1: not wired, the radar cannot be configured from the panel
  // Where this radar sits and points, for unifying its targets with the other two on one shared plane (radar_calibration.hpp). Millimetres and
  // degrees; a tracker left at all zeros publishes exactly what it always has (no rotation, no offset).
  int offset_x_mm = 0;   // -5000..5000: this radar's position versus the node's own origin
  int offset_y_mm = 0;   // -5000..5000
  int yaw_deg = 0;       // -180..180: this radar's own heading versus the node's forward direction (0, 120, 240 for 360 degree coverage)
  int pitch_deg = 0;     // -45..45: tilted down (positive) foreshortens the reported forward distance
  // What the panel has told the sensor itself, kept here so the settings file holds it and the node can tell the sensor again at every start (a
  // module that was reset, or a spare one swapped in, would otherwise silently lose it). All zeros = nothing was ever set: the module is left alone.
  int zone_type = 0;               // 0 no filter, 1 report only what is inside the rectangles, 2 ignore what is inside them
  std::array<ZoneRect, kZonesPerRadar> zones{};
  int target_mode = 0;             // LD2450: 0 leave the module as it is, 1 one target, 2 up to three
  // LD2412 only: the distance gates it detects in (0.75 m each) and how long it waits before it says nobody. gate_max 0: the node leaves the sensor's own values alone. With gates set, the node
  // writes them a few seconds after start and, on the firmware versions that forget them, reads them back about 40 s later and writes them once more if they changed.
  int gate_min = 1;                // 1 to 13
  int gate_max = 0;                // 0: not managed; 1 to 13
  int unmanned_s = 5;              // 0 to 65535
};

struct Sensors {
  bool veml7700 = true;
  int sda = board::kDefaultI2cSda, scl = board::kDefaultI2cScl;
  int lux_fallback = -1;  // -1: withhold telemetry without a light reading
};

// One mapped pin: something the server can read (an input, an analogue value) or switch (an output, a PWM level).
struct MappedPin {
  int gpio = -1;
  std::string name;  // a-z, 0-9, '_': becomes a piece of the MQTT topic
  PinMode mode = PinMode::kDisabled;
  bool invert = false;     // input/output: "on" is a low level
  Pull pull = Pull::kNone;  // input only
  bool initial_on = false;  // output: the level at start
  SafeState safe = SafeState::kKeep;  // output: the state to fall back to when the server is silent for link_timeout_s
  int link_timeout_s = 0;   // 0: never fall back
  int pulse_ms = 0;         // output: a "pulse" command without a length lasts this long (0: 500)
  int debounce_ms = 30;     // input
  int period_s = 10;        // analogue: how often the value is published
  int freq_hz = 1000;       // pwm
  // The field of the device state this pin fills in when it reports to the server (see report_of); empty: the default of its mode.
  std::string report;
  double scale = 1.0, offset = 0.0;  // analogue: reported value = millivolts * scale + offset
};

// The report field of a pin and the ones its mode allows. The names are the canonical fields of A.R.M.O.R.'s device layer; "mv" is
// the raw millivolts at the pin, for an analogue value the server maps itself.
inline const char* default_report(PinMode mode) {
  switch (mode) { case PinMode::kInput: return "triggered"; case PinMode::kOutput: return "on"; case PinMode::kPwm: return "brightness"; case PinMode::kAdc: return "mv"; case PinMode::kDisabled: break; }
  return "";
}
inline bool report_allowed(PinMode mode, std::string_view field) {
  const auto in = [&](std::initializer_list<const char*> list) { for (const char* item : list) if (field == item) return true; return false; };
  switch (mode) {
    case PinMode::kInput: return in({"triggered", "open", "on", "tamper"});
    case PinMode::kOutput: return in({"on", "locked"});
    case PinMode::kPwm: return in({"brightness"});
    case PinMode::kAdc: return in({"mv", "battery", "brightness", "humidity", "temperature", "lux", "power_w", "co_ppm"});
    case PinMode::kDisabled: break;
  }
  return false;
}
inline std::string report_of(const MappedPin& pin) { return pin.report.empty() ? default_report(pin.mode) : pin.report; }

// What time the node believes it is: a time server on the Internet (or the browser's clock when that is off) and the zone the local time is
// shown in. The zone is a POSIX TZ rule, so summer time changes by itself ("CET-1CEST,M3.5.0,M10.5.0/3" is Spain); "UTC0" is no offset.
struct Clock {
  bool ntp_enabled = true;
  std::string ntp = "pool.ntp.org";
  std::string zone = "UTC0";
};

struct Settings {
  std::string node_id;
  std::string node_name;
  Uplink uplink = board::kHasEthernet ? Uplink::kEthernet : Uplink::kWifi;   // the board's own way in until the panel says otherwise
  IpSettings ip;
  AccessPoint ap;
  Station sta;
  Mqtt mqtt;
  Clock time;
  std::array<RadarLine, kRadarCount> radars;
  Sensors sensors;
  std::vector<MappedPin> pins;
  BleMode ble = BleMode::kSetup;
  WebMode web = WebMode::kBoth;
  std::string admin_sources;   // the networks that may use the panel; empty = the private ranges of a house (see admin_sources.hpp)
  std::string language = "en";
  // A periodic, unconditional restart (disconnect_before_restart() then esp_restart()), independent of any fault: 0 means never. One of
  // {0, 1, 2, 3, 4, 6, 12, 24, 48} hours (auto_restart_hours_is_valid()).
  int auto_restart_hours = 0;
  // Wrong passwords the whole node lets through in a minute, from all addresses and names together (auth::LoginThrottle). 30 is how the node has always behaved; 0 turns this limit off (the limit of
  // each address and each person stays); at most 120.
  int login_limit_per_minute = 30;
  // Keep the secrets this node stores (the passwords of its networks and users, tokens, keys, PINs) sealed in its flash (core/secret_seal.hpp). Off, as the node always was: they are readable text. Turning it on needs no
  // eFuse and does not encrypt the flash, so it does not protect against someone who reads the whole chip; the panel says so.
  bool seal_secrets = false;
  // How the task that reads the sensors waits for them (core/uart_wake.hpp): 0 looks at the serial lines every 10 ms, as the first versions did; 10 to 500 sleeps until a sensor sends
  // something and, when none does, at most this many milliseconds before it does its periodic work. 100 by default.
  int reader_idle_ms = uartwake::kIdleMsDefault;
  // Two radars seeing one person at once, within this many millimetres of each other on the shared plane (radar_calibration.hpp), are
  // published as one target instead of two. 0: off, published exactly as each radar reported it (the setting's own default).
  int fusion_merge_mm = 0;
  // The node's own perimeter engine (zones, tripwire lines, paths, masking, the pet filter and how the targets are streamed): off until the panel turns it on. See core/perimeter_config.hpp.
  perimeter::Config perimeter;
  // The sensors that have a Bluetooth radio of their own (LD2450, LD2410, LD2412) ship with it on and open. On (the default): the node closes it a few seconds after start and keeps it closed.
  bool sensor_bluetooth_off = true;
};

struct Problem {
  std::string path;  // "ap.ssid"
  std::string code;  // "required", "too_long", "range", "invalid", "conflict", "reserved" ...
};
using Problems = std::vector<Problem>;

inline const char* to_text(Uplink v) { return v == Uplink::kWifi ? "wifi" : "ethernet"; }
inline const char* to_text(WifiSecurity v) {
  switch (v) { case WifiSecurity::kOpen: return "open"; case WifiSecurity::kWpa2: return "wpa2"; case WifiSecurity::kWpa3: return "wpa3"; case WifiSecurity::kWpa2Wpa3: return "wpa2wpa3"; }
  return "wpa2";
}
inline const char* to_text(PinMode v) {
  switch (v) { case PinMode::kDisabled: return "disabled"; case PinMode::kInput: return "input"; case PinMode::kOutput: return "output"; case PinMode::kPwm: return "pwm"; case PinMode::kAdc: return "adc"; }
  return "disabled";
}
inline const char* to_text(Pull v) { return v == Pull::kUp ? "up" : v == Pull::kDown ? "down" : "none"; }
inline const char* to_text(WebMode v) { return v == WebMode::kHttps ? "https" : v == WebMode::kHttp ? "http" : "both"; }
inline const char* to_text(BleMode v) { return v == BleMode::kAlways ? "always" : v == BleMode::kOff ? "off" : "setup"; }
inline const char* to_text(SafeState v) { return v == SafeState::kOn ? "on" : v == SafeState::kOff ? "off" : "keep"; }

// ---- the defaults of a node that has never been configured ---------------------------------------------------------------------

// `mac_tail` is the last three bytes of the node's MAC as six lowercase hexadecimal digits: it makes the first identity unique.
inline Settings default_settings(std::string_view mac_tail) {
  Settings s;
  s.node_id = "armor-" + std::string(mac_tail);
  s.node_name = s.node_id;
  for (std::size_t i = 0; i < kRadarCount; ++i) {
    s.radars[i].enabled = true;
    s.radars[i].rx = board::kDefaultRadarRx[i];
    s.radars[i].tx = board::kDefaultRadarTx[i];
  }
  return s;
}

// The Wi-Fi channel a node uses: its own setting, or (auto) one of the three that do not overlap, chosen from the node's MAC so that
// the access points of a perimeter spread over 1, 6 and 11 without anyone having to plan it.
inline int effective_channel(const AccessPoint& ap, unsigned mac_sum) {
  if (ap.channel >= 1 && ap.channel <= 13) return ap.channel;
  constexpr int kNonOverlapping[3] = {1, 6, 11};
  return kNonOverlapping[mac_sum % 3];
}

// ---- reading -------------------------------------------------------------------------------------------------------------------

namespace detail {
template <typename E>
bool read_choice(const json::Value& parent, const char* name, std::initializer_list<std::pair<const char*, E>> options, E& target) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return true;
  if (!member->is_string()) return false;
  for (const auto& option : options) if (member->text == option.first) { target = option.second; return true; }
  return false;
}

inline void bad(Problems& problems, std::string path, const char* code) { problems.push_back({std::move(path), code}); }

inline void read_text(const json::Value& parent, const char* name, std::string& target, std::size_t longest, const std::string& path, Problems& problems) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_string()) { bad(problems, path, "invalid"); return; }
  if (member->text.size() > longest) { bad(problems, path, "too_long"); return; }
  target = member->text;
}

// A secret: absent or empty keeps the stored one; "<name>_clear": true erases it.
// The host of a broker address ("mqtt://host:1883/path" -> "host"), lower case.
inline std::string broker_host(const std::string& uri) {
  const std::size_t scheme = uri.find("://");
  std::string rest = scheme == std::string::npos ? uri : uri.substr(scheme + 3);
  const std::size_t end = rest.find_first_of(":/");
  if (end != std::string::npos) rest.resize(end);
  for (char& c : rest) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  return rest;
}

inline void read_secret(const json::Value& parent, const char* name, std::string& target, const std::string& path, Problems& problems) {
  if (parent.bool_or(std::string(name) + "_clear", false)) { target.clear(); return; }
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_string()) { bad(problems, path, "invalid"); return; }
  if (member->text.empty()) return;
  if (member->text.size() > kMaxPasswordText) { bad(problems, path, "too_long"); return; }
  target = member->text;
}

inline void read_bool(const json::Value& parent, const char* name, bool& target, const std::string& path, Problems& problems) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_bool()) { bad(problems, path, "invalid"); return; }
  target = member->boolean;
}

inline void read_real(const json::Value& parent, const char* name, double& target, double lowest, double highest, const std::string& path, Problems& problems) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_number()) { bad(problems, path, "invalid"); return; }
  if (member->number < lowest || member->number > highest) { bad(problems, path, "range"); return; }
  target = member->number;
}

inline void read_int(const json::Value& parent, const char* name, int& target, long long lowest, long long highest, const std::string& path, Problems& problems) {
  const json::Value* member = parent.get(name);
  if (member == nullptr) return;
  if (!member->is_number()) { bad(problems, path, "invalid"); return; }
  const long long value = parent.integer_or(name, lowest - 1, lowest, highest);
  if (value < lowest || value > highest) { bad(problems, path, "range"); return; }
  target = static_cast<int>(value);
}
}  // namespace detail

// The perimeter engine's settings. Every member is optional. A list in the document (zones, lines, paths) replaces the stored one, like the other lists.
inline void read_perimeter(const json::Value& p, perimeter::Config& c, Problems& problems) {
  using namespace detail;
  read_bool(p, "enabled", c.enabled, "perimeter.enabled", problems);
  read_int(p, "range_min_mm", c.range_min_mm, -500, 4000, "perimeter.range_min_mm", problems);
  read_int(p, "range_max_mm", c.range_max_mm, 1000, 8000, "perimeter.range_max_mm", problems);
  read_int(p, "debounce_frames", c.debounce_frames, 1, 20, "perimeter.debounce_frames", problems);
  read_int(p, "count_window", c.count_window, 1, 20, "perimeter.count_window", problems);
  read_bool(p, "strict_entry", c.strict_entry, "perimeter.strict_entry", problems);
  read_bool(p, "jam_detection", c.jam_detection, "perimeter.jam_detection", problems);
  if (!read_choice<perimeter::Stream>(p, "stream", {{"live", perimeter::Stream::kLive}, {"change", perimeter::Stream::kOnChange}, {"events", perimeter::Stream::kEventsOnly}}, c.stream)) bad(problems, "perimeter.stream", "invalid");
  if (const json::Value* night = p.get("night"); night != nullptr) {
    if (!night->is_object()) bad(problems, "perimeter.night", "invalid");
    else {
      read_bool(*night, "enabled", c.night_enabled, "perimeter.night.enabled", problems);
      read_int(*night, "from_min", c.night_from_min, 0, 1439, "perimeter.night.from_min", problems);
      read_int(*night, "to_min", c.night_to_min, 0, 1439, "perimeter.night.to_min", problems);
    }
  }
  if (const json::Value* pet = p.get("pet"); pet != nullptr) {
    if (!pet->is_object()) bad(problems, "perimeter.pet", "invalid");
    else {
      read_bool(*pet, "enabled", c.pet_immunity, "perimeter.pet.enabled", problems);
      read_int(*pet, "distance_cm", c.pet_distance_cm, 50, 400, "perimeter.pet.distance_cm", problems);
      read_int(*pet, "energy", c.pet_energy, 1, 100, "perimeter.pet.energy", problems);
    }
  }
  if (const json::Value* masking = p.get("masking"); masking != nullptr) {
    if (!masking->is_object()) bad(problems, "perimeter.masking", "invalid");
    else {
      read_int(*masking, "minutes", c.masking_minutes, 0, 1440, "perimeter.masking.minutes", problems);
      read_bool(*masking, "armed_only", c.masking_armed_only, "perimeter.masking.armed_only", problems);
    }
  }
  if (const json::Value* zones = p.get("zones"); zones != nullptr) {
    if (!zones->is_array() || zones->items.size() > perimeter::kMaxZones) bad(problems, "perimeter.zones", "invalid");
    else {
      for (perimeter::Zone& z : c.zones) z = perimeter::Zone{};
      for (std::size_t i = 0; i < zones->items.size(); ++i) {
        const json::Value& item = zones->items[i];
        const std::string base = "perimeter.zones." + std::to_string(i) + ".";
        perimeter::Zone& z = c.zones[i];
        if (!item.is_object()) { bad(problems, base + "name", "invalid"); continue; }
        read_bool(item, "enabled", z.enabled, base + "enabled", problems);
        read_text(item, "name", z.name, perimeter::kMaxName, base + "name", problems);
        read_bool(item, "exclude", z.exclude, base + "exclude", problems);
        read_int(item, "x_mm", z.x_mm, -12800, 12800, base + "x_mm", problems);
        read_int(item, "y_mm", z.y_mm, -12800, 12800, base + "y_mm", problems);
        read_int(item, "width_mm", z.width_mm, 0, 25600, base + "width_mm", problems);
        read_int(item, "height_mm", z.height_mm, 0, 25600, base + "height_mm", problems);
        read_int(item, "angle_deg", z.angle_deg, -180, 180, base + "angle_deg", problems);
        read_int(item, "profiles", z.profiles, 1, 3, base + "profiles", problems);
        read_bool(item, "latch", z.latch, base + "latch", problems);
        read_int(item, "loiter_s", z.loiter_s, 0, 86400, base + "loiter_s", problems);
        read_bool(item, "active", z.active, base + "active", problems);
      }
    }
  }
  if (const json::Value* lines = p.get("lines"); lines != nullptr) {
    if (!lines->is_array() || lines->items.size() > perimeter::kMaxLines) bad(problems, "perimeter.lines", "invalid");
    else {
      for (perimeter::Line& l : c.lines) l = perimeter::Line{};
      for (std::size_t i = 0; i < lines->items.size(); ++i) {
        const json::Value& item = lines->items[i];
        const std::string base = "perimeter.lines." + std::to_string(i) + ".";
        perimeter::Line& l = c.lines[i];
        if (!item.is_object()) { bad(problems, base + "name", "invalid"); continue; }
        read_bool(item, "enabled", l.enabled, base + "enabled", problems);
        read_text(item, "name", l.name, perimeter::kMaxName, base + "name", problems);
        read_int(item, "ax_mm", l.ax_mm, -12800, 12800, base + "ax_mm", problems);
        read_int(item, "ay_mm", l.ay_mm, -12800, 12800, base + "ay_mm", problems);
        read_int(item, "bx_mm", l.bx_mm, -12800, 12800, base + "bx_mm", problems);
        read_int(item, "by_mm", l.by_mm, -12800, 12800, base + "by_mm", problems);
      }
    }
  }
  if (const json::Value* paths = p.get("paths"); paths != nullptr) {
    if (!paths->is_array() || paths->items.size() > perimeter::kMaxPaths) bad(problems, "perimeter.paths", "invalid");
    else {
      for (perimeter::Path& q : c.paths) q = perimeter::Path{};
      for (std::size_t i = 0; i < paths->items.size(); ++i) {
        const json::Value& item = paths->items[i];
        const std::string base = "perimeter.paths." + std::to_string(i) + ".";
        perimeter::Path& q = c.paths[i];
        if (!item.is_object()) { bad(problems, base + "name", "invalid"); continue; }
        read_bool(item, "enabled", q.enabled, base + "enabled", problems);
        read_text(item, "name", q.name, perimeter::kMaxName, base + "name", problems);
        if (const json::Value* list = item.get("zones"); list != nullptr) {
          if (!list->is_array() || list->items.size() > perimeter::kPathLength) { bad(problems, base + "zones", "invalid"); continue; }
          for (std::string& name : q.zones) name.clear();
          for (std::size_t k = 0; k < list->items.size(); ++k) {
            if (!list->items[k].is_string() || list->items[k].text.size() > perimeter::kMaxName) { bad(problems, base + "zones." + std::to_string(k), "invalid"); continue; }
            q.zones[k] = list->items[k].text;
          }
        }
      }
    }
  }
}

// Applies the members present in `document` on top of `settings`; whatever the document omits stays as it was. The result is checked
// with validate(): read_settings() only reports a member of the wrong type or size.
inline void read_settings(const json::Value& document, Settings& s, Problems& problems) {
  using namespace detail;
  if (!document.is_object()) { bad(problems, "", "invalid"); return; }
  if (const json::Value* node = document.get("node"); node != nullptr && node->is_object()) {
    read_text(*node, "id", s.node_id, kMaxNodeIdLength, "node.id", problems);
    read_text(*node, "name", s.node_name, 48, "node.name", problems);
  }
  if (document.get("uplink") != nullptr && !read_choice<Uplink>(document, "uplink", {{"ethernet", Uplink::kEthernet}, {"wifi", Uplink::kWifi}}, s.uplink)) bad(problems, "uplink", "invalid");
  if (const json::Value* ip = document.get("ip"); ip != nullptr && ip->is_object()) {
    read_bool(*ip, "dhcp", s.ip.dhcp, "ip.dhcp", problems);
    read_text(*ip, "address", s.ip.address, 15, "ip.address", problems);
    read_text(*ip, "netmask", s.ip.netmask, 15, "ip.netmask", problems);
    read_text(*ip, "gateway", s.ip.gateway, 15, "ip.gateway", problems);
    read_text(*ip, "dns1", s.ip.dns1, 15, "ip.dns1", problems);
    read_text(*ip, "dns2", s.ip.dns2, 15, "ip.dns2", problems);
    read_text(*ip, "hostname", s.ip.hostname, 32, "ip.hostname", problems);
  }
  if (const json::Value* ap = document.get("ap"); ap != nullptr && ap->is_object()) {
    read_bool(*ap, "enabled", s.ap.enabled, "ap.enabled", problems);
    read_text(*ap, "ssid", s.ap.ssid, 32, "ap.ssid", problems);
    if (!read_choice<WifiSecurity>(*ap, "security", {{"open", WifiSecurity::kOpen}, {"wpa2", WifiSecurity::kWpa2}, {"wpa3", WifiSecurity::kWpa3}, {"wpa2wpa3", WifiSecurity::kWpa2Wpa3}}, s.ap.security)) bad(problems, "ap.security", "invalid");
    read_secret(*ap, "password", s.ap.password, "ap.password", problems);
    read_int(*ap, "channel", s.ap.channel, 0, 13, "ap.channel", problems);
    read_bool(*ap, "hidden", s.ap.hidden, "ap.hidden", problems);
    read_int(*ap, "max_clients", s.ap.max_clients, 1, 10, "ap.max_clients", problems);
    read_int(*ap, "tx_power_dbm", s.ap.tx_power_dbm, 2, 20, "ap.tx_power_dbm", problems);
    {   // 20 or 40 MHz only: anything between would be run as 20 by the radio, so it is refused rather than accepted and ignored
      int bandwidth = s.ap.bandwidth_mhz;
      read_int(*ap, "bandwidth_mhz", bandwidth, 20, 40, "ap.bandwidth_mhz", problems);
      if (bandwidth == 20 || bandwidth == 40) s.ap.bandwidth_mhz = bandwidth;
      else if (bandwidth != s.ap.bandwidth_mhz) bad(problems, "ap.bandwidth_mhz", "range");
    }
    read_text(*ap, "country", s.ap.country, 2, "ap.country", problems);
    read_bool(*ap, "bridge", s.ap.bridge, "ap.bridge", problems);
  }
  if (const json::Value* sta = document.get("sta"); sta != nullptr && sta->is_object()) {
    read_bool(*sta, "enabled", s.sta.enabled, "sta.enabled", problems);
    read_text(*sta, "ssid", s.sta.ssid, 32, "sta.ssid", problems);
    read_secret(*sta, "password", s.sta.password, "sta.password", problems);
    if (const json::Value* backup = sta->get("backup"); backup != nullptr) {
      if (!backup->is_array()) bad(problems, "sta.backup", "invalid");
      // A "backup" the document sends replaces the stored list, never adds to it - a save after removing one in the panel must not
      // leave the one just removed behind (found for real: deleting both backup brokers and saving brought them straight back).
      else {
        // The panel never holds a stored password (it only learns that there is one), so an entry that arrives without one is the same
        // network as before and keeps its password; one with a new name is a different network and starts without.
        const std::vector<Network> before = std::move(s.sta.backup);
        s.sta.backup.clear();
        // More than fit is never fatal: an older or hand-edited document with extra entries loses only the ones past the limit, not
        // the whole node (a broker and a radar line have nothing to do with how many backup networks were once saved).
        for (std::size_t i = 0; i < backup->items.size() && i < kMaxBackupNetworks; ++i) {
          const json::Value& item = backup->items[i];
          const std::string base = "sta.backup." + std::to_string(i) + ".";
          Network network;
          if (!item.is_object()) { bad(problems, base + "ssid", "invalid"); continue; }
          read_text(item, "ssid", network.ssid, 32, base + "ssid", problems);
          for (const Network& old : before) if (old.ssid == network.ssid) { network.password = old.password; break; }
          read_secret(item, "password", network.password, base + "password", problems);
          s.sta.backup.push_back(network);
        }
      }
    }
  }
  if (const json::Value* mqtt = document.get("mqtt"); mqtt != nullptr && mqtt->is_object()) {
    read_bool(*mqtt, "enabled", s.mqtt.enabled, "mqtt.enabled", problems);
    const std::string stored_broker_host = broker_host(s.mqtt.uri);
    read_text(*mqtt, "uri", s.mqtt.uri, 160, "mqtt.uri", problems);
    read_text(*mqtt, "username", s.mqtt.username, 64, "mqtt.username", problems);
    // The stored password follows the broker it was made for: pointing the node at another host without typing a password does not send the old one there (a stolen session could
    // otherwise read it out by naming a broker of its own).
    if (broker_host(s.mqtt.uri) != stored_broker_host) s.mqtt.password.clear();
    read_secret(*mqtt, "password", s.mqtt.password, "mqtt.password", problems);
    read_int(*mqtt, "heartbeat_s", s.mqtt.heartbeat_s, 2, 300, "mqtt.heartbeat_s", problems);
    read_bool(*mqtt, "tls", s.mqtt.tls, "mqtt.tls", problems);
    if (mqtt->get("ca_pem") != nullptr) {
      read_text(*mqtt, "ca_pem", s.mqtt.ca_pem, mqtt_tls::kMaxCaPem, "mqtt.ca_pem", problems);
      s.mqtt.ca_pem = mqtt_tls::normalize(s.mqtt.ca_pem);
    }
    read_int(*mqtt, "telemetry_ms", s.mqtt.telemetry_ms, 200, 5000, "mqtt.telemetry_ms", problems);
    read_text(*mqtt, "ntp", s.time.ntp, 64, "mqtt.ntp", problems);   // where older documents kept it
    if (const json::Value* backup = mqtt->get("backup"); backup != nullptr) {
      if (!backup->is_array()) bad(problems, "mqtt.backup", "invalid");
      // A "backup" the document sends replaces the stored list, never adds to it (see sta.backup above - the same bug, found on the
      // broker page: removing both backup brokers and saving brought them straight back).
      else {
        // Same as the backup networks: an entry that arrives without a password is the same account as before (same user, same host,
        // on the same slot or at the same address) and keeps it; one pointed at another host starts without.
        const std::vector<Broker> before = std::move(s.mqtt.backup);
        s.mqtt.backup.clear();
        // Same as sta.backup above: more entries than fit just lose the extras, not the rest of the node's settings.
        for (std::size_t i = 0; i < backup->items.size() && i < kMaxBackupBrokers; ++i) {
          const json::Value& item = backup->items[i];
          const std::string base = "mqtt.backup." + std::to_string(i) + ".";
          Broker broker;
          if (!item.is_object()) { bad(problems, base + "uri", "invalid"); continue; }
          read_text(item, "uri", broker.uri, 160, base + "uri", problems);
          read_text(item, "username", broker.username, 64, base + "username", problems);
          for (std::size_t k = 0; k < before.size(); ++k) {
            if (before[k].username == broker.username && broker_host(before[k].uri) == broker_host(broker.uri) && (k == i || before[k].uri == broker.uri)) { broker.password = before[k].password; break; }
          }
          read_secret(item, "password", broker.password, base + "password", problems);
          s.mqtt.backup.push_back(broker);
        }
      }
    }
  }
  if (const json::Value* radars = document.get("radars"); radars != nullptr) {
    if (!radars->is_array() || radars->items.size() > kRadarCount) bad(problems, "radars", "invalid");
    else for (std::size_t i = 0; i < radars->items.size(); ++i) {
      const json::Value& item = radars->items[i];
      const std::string base = "radars." + std::to_string(i) + ".";
      if (!item.is_object()) { bad(problems, base + "enabled", "invalid"); continue; }
      read_bool(item, "enabled", s.radars[i].enabled, base + "enabled", problems);
      read_text(item, "model", s.radars[i].model, 12, base + "model", problems);
      read_int(item, "baud", s.radars[i].baud, 0, 921600, base + "baud", problems);
      read_text(item, "name", s.radars[i].name, 24, base + "name", problems);
      read_int(item, "rx", s.radars[i].rx, -1, board::kLastGpio, base + "rx", problems);
      read_int(item, "tx", s.radars[i].tx, -1, board::kLastGpio, base + "tx", problems);
      read_int(item, "offset_x_mm", s.radars[i].offset_x_mm, -5000, 5000, base + "offset_x_mm", problems);
      read_int(item, "offset_y_mm", s.radars[i].offset_y_mm, -5000, 5000, base + "offset_y_mm", problems);
      read_int(item, "yaw_deg", s.radars[i].yaw_deg, -180, 180, base + "yaw_deg", problems);
      read_int(item, "pitch_deg", s.radars[i].pitch_deg, -45, 45, base + "pitch_deg", problems);
      read_int(item, "target_mode", s.radars[i].target_mode, 0, 2, base + "target_mode", problems);
      read_int(item, "gate_min", s.radars[i].gate_min, 1, 13, base + "gate_min", problems);
      read_int(item, "gate_max", s.radars[i].gate_max, 0, 13, base + "gate_max", problems);
      read_int(item, "unmanned_s", s.radars[i].unmanned_s, 0, 65535, base + "unmanned_s", problems);
      if (const json::Value* zones = item.get("zones"); zones != nullptr) {
        if (!zones->is_object()) bad(problems, base + "zones", "invalid");
        else {
          read_int(*zones, "type", s.radars[i].zone_type, 0, 2, base + "zones.type", problems);
          if (const json::Value* list = zones->get("list"); list != nullptr) {
            if (!list->is_array() || list->items.size() > kZonesPerRadar) bad(problems, base + "zones.list", "invalid");
            else for (std::size_t z = 0; z < list->items.size(); ++z) {
              const json::Value& rect = list->items[z];
              const std::string where = base + "zones.list." + std::to_string(z) + ".";
              if (!rect.is_object()) { bad(problems, where + "x1", "invalid"); continue; }
              read_int(rect, "x1", s.radars[i].zones[z].x1, -12800, 12800, where + "x1", problems);
              read_int(rect, "y1", s.radars[i].zones[z].y1, -12800, 12800, where + "y1", problems);
              read_int(rect, "x2", s.radars[i].zones[z].x2, -12800, 12800, where + "x2", problems);
              read_int(rect, "y2", s.radars[i].zones[z].y2, -12800, 12800, where + "y2", problems);
            }
          }
        }
      }
    }
  }
  if (const json::Value* time = document.get("time"); time != nullptr && time->is_object()) {
    read_bool(*time, "ntp_enabled", s.time.ntp_enabled, "time.ntp_enabled", problems);
    read_text(*time, "ntp", s.time.ntp, 64, "time.ntp", problems);
    read_text(*time, "zone", s.time.zone, 48, "time.zone", problems);
  }
  if (const json::Value* sensors = document.get("sensors"); sensors != nullptr && sensors->is_object()) {
    read_bool(*sensors, "veml7700", s.sensors.veml7700, "sensors.veml7700", problems);
    read_int(*sensors, "sda", s.sensors.sda, 0, board::kLastGpio, "sensors.sda", problems);
    read_int(*sensors, "scl", s.sensors.scl, 0, board::kLastGpio, "sensors.scl", problems);
    read_int(*sensors, "lux_fallback", s.sensors.lux_fallback, -1, 200000, "sensors.lux_fallback", problems);
  }
  if (const json::Value* pins = document.get("pins"); pins != nullptr) {
    if (!pins->is_array() || pins->items.size() > kMaxMappedPins) { bad(problems, "pins", pins->is_array() ? "too_many" : "invalid"); }
    else {
      s.pins.clear();
      for (std::size_t i = 0; i < pins->items.size(); ++i) {
        const json::Value& item = pins->items[i];
        const std::string base = "pins." + std::to_string(i) + ".";
        MappedPin pin;
        if (!item.is_object()) { bad(problems, base + "gpio", "invalid"); continue; }
        read_int(item, "gpio", pin.gpio, 0, board::kLastGpio, base + "gpio", problems);
        read_text(item, "name", pin.name, 24, base + "name", problems);
        if (!read_choice<PinMode>(item, "mode", {{"disabled", PinMode::kDisabled}, {"input", PinMode::kInput}, {"output", PinMode::kOutput}, {"pwm", PinMode::kPwm}, {"adc", PinMode::kAdc}}, pin.mode)) bad(problems, base + "mode", "invalid");
        read_bool(item, "invert", pin.invert, base + "invert", problems);
        if (!read_choice<Pull>(item, "pull", {{"none", Pull::kNone}, {"up", Pull::kUp}, {"down", Pull::kDown}}, pin.pull)) bad(problems, base + "pull", "invalid");
        read_bool(item, "initial_on", pin.initial_on, base + "initial_on", problems);
        if (!read_choice<SafeState>(item, "safe", {{"off", SafeState::kOff}, {"on", SafeState::kOn}, {"keep", SafeState::kKeep}}, pin.safe)) bad(problems, base + "safe", "invalid");
        read_int(item, "link_timeout_s", pin.link_timeout_s, 0, 86400, base + "link_timeout_s", problems);
        read_int(item, "pulse_ms", pin.pulse_ms, 0, 3600000, base + "pulse_ms", problems);
        read_int(item, "debounce_ms", pin.debounce_ms, 0, 5000, base + "debounce_ms", problems);
        read_int(item, "period_s", pin.period_s, 1, 3600, base + "period_s", problems);
        read_int(item, "freq_hz", pin.freq_hz, 1, 40000, base + "freq_hz", problems);
        read_text(item, "report", pin.report, 12, base + "report", problems);
        read_real(item, "scale", pin.scale, -1000000.0, 1000000.0, base + "scale", problems);
        read_real(item, "offset", pin.offset, -1000000.0, 1000000.0, base + "offset", problems);
        s.pins.push_back(pin);
      }
    }
  }
  if (const json::Value* ble = document.get("ble"); ble != nullptr && ble->is_object()) {
    if (!read_choice<BleMode>(*ble, "mode", {{"off", BleMode::kOff}, {"setup", BleMode::kSetup}, {"always", BleMode::kAlways}}, s.ble)) bad(problems, "ble.mode", "invalid");
  }
  if (const json::Value* web = document.get("web"); web != nullptr && web->is_object()) {
    if (!read_choice<WebMode>(*web, "mode", {{"http", WebMode::kHttp}, {"both", WebMode::kBoth}, {"https", WebMode::kHttps}}, s.web)) bad(problems, "web.mode", "invalid");
    if (web->get("admin_sources") != nullptr) {
      std::string sources = s.admin_sources;
      read_text(*web, "admin_sources", sources, adminsrc::kMaxText, "web.admin_sources", problems);
      if (!adminsrc::valid_list(sources)) bad(problems, "web.admin_sources", "invalid"); else s.admin_sources = sources;
    }
  }
  if (const json::Value* ui = document.get("ui"); ui != nullptr && ui->is_object()) read_text(*ui, "language", s.language, 4, "ui.language", problems);
  if (const json::Value* system = document.get("system"); system != nullptr && system->is_object()) {
    read_int(*system, "auto_restart_hours", s.auto_restart_hours, 0, 48, "system.auto_restart_hours", problems);
    read_int(*system, "login_limit_per_minute", s.login_limit_per_minute, 0, 120, "system.login_limit_per_minute", problems);
    if (system->get("reader_idle_ms") != nullptr) {
      int idle = s.reader_idle_ms;
      read_int(*system, "reader_idle_ms", idle, 0, uartwake::kIdleMsMax, "system.reader_idle_ms", problems);
      if (!uartwake::idle_ms_is_valid(idle)) bad(problems, "system.reader_idle_ms", "range"); else s.reader_idle_ms = idle;
    }
  }
  if (const json::Value* security = document.get("security"); security != nullptr && security->is_object()) read_bool(*security, "seal_secrets", s.seal_secrets, "security.seal_secrets", problems);
  if (const json::Value* fusion = document.get("fusion"); fusion != nullptr && fusion->is_object()) read_int(*fusion, "merge_mm", s.fusion_merge_mm, 0, 2000, "fusion.merge_mm", problems);
  if (const json::Value* p = document.get("perimeter"); p != nullptr) {
    if (!p->is_object()) bad(problems, "perimeter", "invalid"); else read_perimeter(*p, s.perimeter, problems);
  }
  if (const json::Value* security = document.get("sensor_security"); security != nullptr && security->is_object()) read_bool(*security, "bluetooth_off", s.sensor_bluetooth_off, "sensor_security.bluetooth_off", problems);
}

// ---- checking ------------------------------------------------------------------------------------------------------------------

inline bool language_is_known(std::string_view code) {
  for (const char* known : {"en", "es", "de", "fr", "it", "ja", "zh", "pt"}) if (code == known) return true;
  return false;
}

// A POSIX TZ rule: letters, digits, signs, commas, dots, colons, slashes and angle brackets, nothing else (it goes to setenv()).
inline bool time_zone_is_valid(std::string_view zone) {
  if (zone.empty() || zone.size() > 48) return false;
  for (const char c : zone) {
    const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == ',' || c == '.' || c == ':' || c == '/' || c == '<' || c == '>';
    if (!ok) return false;
  }
  return true;
}

inline bool auto_restart_hours_is_valid(int hours) {
  for (const int known : {0, 1, 2, 3, 4, 6, 12, 24, 48}) if (hours == known) return true;
  return false;
}

inline bool valid_slug(std::string_view name) {
  if (name.empty() || name.size() > 24) return false;
  for (const char c : name) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
  return true;
}

inline bool broker_uri_is_valid(std::string_view uri) {
  std::string_view rest;
  if (uri.substr(0, 7) == "mqtt://") rest = uri.substr(7);
  else if (uri.substr(0, 8) == "mqtts://") rest = uri.substr(8);
  else return false;
  const std::size_t colon = rest.find(':');
  const std::string_view host = rest.substr(0, colon);
  if (!net::valid_host(host)) return false;
  if (colon == std::string_view::npos) return true;
  const std::string_view port = rest.substr(colon + 1);
  if (port.empty() || port.size() > 5) return false;
  unsigned value = 0;
  for (const char c : port) { if (c < '0' || c > '9') return false; value = value * 10 + static_cast<unsigned>(c - '0'); }
  return value >= 1 && value <= 65535;
}

namespace detail {
// The pins already claimed, to catch two uses of one GPIO.
struct PinClaims {
  std::array<std::string, board::kLastGpio + 1> owner;
  bool claim(int gpio, const std::string& who, std::string& other) {
    if (gpio < 0 || gpio > board::kLastGpio) return true;
    if (!owner[static_cast<std::size_t>(gpio)].empty()) { other = owner[static_cast<std::size_t>(gpio)]; return false; }
    owner[static_cast<std::size_t>(gpio)] = who;
    return true;
  }
};
}  // namespace detail

// The perimeter engine's settings against each other: the sizes, the names (unique among the zones that are on), the counts of each kind, the points of a line and the zones a path names.
inline void validate_perimeter(const perimeter::Config& c, Problems& problems) {
  using detail::bad;
  if (c.range_max_mm - c.range_min_mm < 500) bad(problems, "perimeter.range_max_mm", "range");
  std::size_t detect = 0, exclude = 0;
  for (std::size_t i = 0; i < perimeter::kMaxZones; ++i) {
    const perimeter::Zone& z = c.zones[i];
    if (!z.enabled) continue;
    const std::string base = "perimeter.zones." + std::to_string(i) + ".";
    if (!perimeter::name_is_valid(z.name)) bad(problems, base + "name", z.name.empty() ? "required" : "invalid");
    for (std::size_t k = 0; k < i; ++k) if (c.zones[k].enabled && c.zones[k].name == z.name) bad(problems, base + "name", "conflict");
    if (z.width_mm < 100) bad(problems, base + "width_mm", "range");
    if (z.height_mm < 100) bad(problems, base + "height_mm", "range");
    if (z.exclude) ++exclude; else ++detect;
    if (z.exclude && (z.latch || z.loiter_s != 0)) bad(problems, base + "latch", "conflict");   // an exclusion zone only hides what is inside it
  }
  if (detect > perimeter::kMaxDetectZones) bad(problems, "perimeter.zones", "too_many_detection");
  if (exclude > perimeter::kMaxExcludeZones) bad(problems, "perimeter.zones", "too_many_exclusion");
  for (std::size_t i = 0; i < perimeter::kMaxLines; ++i) {
    const perimeter::Line& l = c.lines[i];
    if (!l.enabled) continue;
    const std::string base = "perimeter.lines." + std::to_string(i) + ".";
    if (!perimeter::name_is_valid(l.name)) bad(problems, base + "name", l.name.empty() ? "required" : "invalid");
    for (std::size_t k = 0; k < i; ++k) if (c.lines[k].enabled && c.lines[k].name == l.name) bad(problems, base + "name", "conflict");
    const long long dx = l.bx_mm - l.ax_mm, dy = l.by_mm - l.ay_mm;
    if (dx * dx + dy * dy < 200LL * 200LL) bad(problems, base + "bx_mm", "range");   // a line shorter than 20 cm is a point
  }
  for (std::size_t i = 0; i < perimeter::kMaxPaths; ++i) {
    const perimeter::Path& q = c.paths[i];
    if (!q.enabled) continue;
    const std::string base = "perimeter.paths." + std::to_string(i) + ".";
    if (!perimeter::name_is_valid(q.name)) bad(problems, base + "name", q.name.empty() ? "required" : "invalid");
    std::size_t named = 0;
    for (std::size_t k = 0; k < perimeter::kPathLength; ++k) {
      if (q.zones[k].empty()) { for (std::size_t m = k; m < perimeter::kPathLength; ++m) if (!q.zones[m].empty()) bad(problems, base + "zones." + std::to_string(m), "invalid"); break; }
      ++named;
      bool found = false;
      for (const perimeter::Zone& z : c.zones) if (z.enabled && !z.exclude && z.name == q.zones[k]) found = true;
      if (!found) bad(problems, base + "zones." + std::to_string(k), "unknown_zone");
    }
    if (named < 2) bad(problems, base + "zones", "required");
  }
}

inline Problems validate(const Settings& s) {
  using detail::bad;
  Problems problems;
  if (!node_id_is_valid(s.node_id)) bad(problems, "node.id", "invalid");
  std::size_t name_characters = 0;
  if (s.node_name.empty()) bad(problems, "node.name", "required");
  else if (!net::valid_utf8(s.node_name, &name_characters) || name_characters > 48) bad(problems, "node.name", "invalid");
  if (!language_is_known(s.language)) bad(problems, "ui.language", "invalid");
  if (!auto_restart_hours_is_valid(s.auto_restart_hours)) bad(problems, "system.auto_restart_hours", "invalid");
  if (!uartwake::idle_ms_is_valid(s.reader_idle_ms)) bad(problems, "system.reader_idle_ms", "range");

  // network
  if (s.uplink == Uplink::kEthernet && !board::kHasEthernet) bad(problems, "uplink", "not_available");   // this board has no cable
  if (!s.ip.dhcp) {   // a fixed address is for whichever connection the node uses, the cable or the Wi-Fi station
    std::uint32_t address = 0, mask = 0, gateway = 0, dns = 0;
    const bool address_ok = net::parse_ipv4(s.ip.address, address), mask_ok = net::parse_ipv4(s.ip.netmask, mask) && net::valid_netmask(mask);
    if (!address_ok || !net::usable_host_address(address)) bad(problems, "ip.address", s.ip.address.empty() ? "required" : "invalid");
    if (!mask_ok) bad(problems, "ip.netmask", s.ip.netmask.empty() ? "required" : "invalid");
    if (address_ok && mask_ok && net::is_network_or_broadcast(address, mask)) bad(problems, "ip.address", "invalid");
    if (!net::parse_ipv4(s.ip.gateway, gateway) || !net::usable_host_address(gateway)) bad(problems, "ip.gateway", s.ip.gateway.empty() ? "required" : "invalid");
    else if (address_ok && mask_ok && !net::same_subnet(address, gateway, mask)) bad(problems, "ip.gateway", "outside_subnet");
    else if (address_ok && gateway == address) bad(problems, "ip.gateway", "conflict");
    if (!s.ip.dns1.empty() && !net::parse_ipv4(s.ip.dns1, dns)) bad(problems, "ip.dns1", "invalid");
    if (!s.ip.dns2.empty() && !net::parse_ipv4(s.ip.dns2, dns)) bad(problems, "ip.dns2", "invalid");
  }
  if (!s.ip.hostname.empty() && !net::valid_hostname(s.ip.hostname)) bad(problems, "ip.hostname", "invalid");

  // Wi-Fi
  if (s.ap.enabled) {
    if (!net::valid_ssid(s.ap.ssid)) bad(problems, "ap.ssid", s.ap.ssid.empty() ? "required" : "invalid");
    if (s.ap.security != WifiSecurity::kOpen && !net::valid_wpa_passphrase(s.ap.password)) bad(problems, "ap.password", s.ap.password.empty() ? "required" : "invalid_key");
  }
  if (s.ap.country.size() != 2 || !(s.ap.country[0] >= 'A' && s.ap.country[0] <= 'Z' && s.ap.country[1] >= 'A' && s.ap.country[1] <= 'Z')) bad(problems, "ap.country", "invalid");
  if (s.sta.enabled) {
    if (!net::valid_ssid(s.sta.ssid)) bad(problems, "sta.ssid", s.sta.ssid.empty() ? "required" : "invalid");
    if (!s.sta.password.empty() && !net::valid_wpa_passphrase(s.sta.password)) bad(problems, "sta.password", "invalid_key");
    for (std::size_t i = 0; i < s.sta.backup.size(); ++i) {
      const std::string base = "sta.backup." + std::to_string(i) + ".";
      const Network& network = s.sta.backup[i];
      if (!net::valid_ssid(network.ssid)) bad(problems, base + "ssid", network.ssid.empty() ? "required" : "invalid");
      if (!network.password.empty() && !net::valid_wpa_passphrase(network.password)) bad(problems, base + "password", "invalid_key");
    }
  }
  if (s.uplink == Uplink::kWifi && !s.sta.enabled) bad(problems, "sta.enabled", "required");


  // clock
  if (s.time.ntp_enabled && !net::valid_host(s.time.ntp)) bad(problems, "time.ntp", "invalid");
  if (!time_zone_is_valid(s.time.zone)) bad(problems, "time.zone", "invalid");

  // broker
  mqtt_tls::validate(s.mqtt, problems);
  if (s.mqtt.enabled) {
    if (s.mqtt.uri.empty()) bad(problems, "mqtt.uri", "required");
    else if (!broker_uri_is_valid(s.mqtt.uri)) bad(problems, "mqtt.uri", "invalid");
    for (std::size_t i = 0; i < s.mqtt.backup.size(); ++i) {
      const std::string base = "mqtt.backup." + std::to_string(i) + ".";
      if (!broker_uri_is_valid(s.mqtt.backup[i].uri)) bad(problems, base + "uri", s.mqtt.backup[i].uri.empty() ? "required" : "invalid");
    }
  }

  // pins: the radars, the light sensor and the mapped pins may not share a GPIO, and none may be a reserved one
  std::vector<std::string> names;   // the names of the devices this node offers (presence sensors and mapped pins share one namespace)
  detail::PinClaims claims;
  const auto claim = [&](int gpio, const std::string& who, const std::string& path, bool allow_sd) {
    if (!board::assignable(gpio, allow_sd)) { bad(problems, path, board::pin_info(gpio).use == board::PinUse::kSdCard ? "sd_card" : "reserved"); return; }
    std::string other;
    if (!claims.claim(gpio, who, other)) bad(problems, path, "conflict");
  };
  for (std::size_t i = 0; i < kRadarCount; ++i) {
    const RadarLine& radar = s.radars[i];
    if (!radar.enabled) continue;
    const std::string base = "radars." + std::to_string(i) + ".", who = "radar" + std::to_string(i + 1);
    sensors::Model model;
    if (!sensors::from_text(radar.model, model)) bad(problems, base + "model", "invalid");
    else {
      static const int kSpeeds[] = {9600, 19200, 38400, 57600, 115200, 230400, 256000, 460800};
      if (radar.baud != 0 && std::find(std::begin(kSpeeds), std::end(kSpeeds), radar.baud) == std::end(kSpeeds)) bad(problems, base + "baud", "range");
      if (radar.gate_max != 0 && (model != sensors::Model::kLd2412 || radar.gate_min > radar.gate_max)) bad(problems, base + "gate_max", model != sensors::Model::kLd2412 ? "unsupported" : "range");
      if (!sensors::info(model).tracker) {   // a presence sensor is a device: it needs a name for its topic
        if (!valid_slug(radar.name)) bad(problems, base + "name", radar.name.empty() ? "required" : "invalid");
        else if (std::find(names.begin(), names.end(), radar.name) != names.end()) bad(problems, base + "name", "conflict");
        else names.push_back(radar.name);
      }
    }
    if (radar.rx < 0) bad(problems, base + "rx", "required"); else claim(radar.rx, who, base + "rx", true);
    if (radar.tx >= 0) claim(radar.tx, who, base + "tx", true);
  }
  if (s.sensors.veml7700) {
    claim(s.sensors.sda, "i2c", "sensors.sda", true);
    claim(s.sensors.scl, "i2c", "sensors.scl", true);
  }
  for (std::size_t i = 0; i < s.pins.size(); ++i) {
    const MappedPin& pin = s.pins[i];
    const std::string base = "pins." + std::to_string(i) + ".";
    if (pin.mode == PinMode::kDisabled) continue;
    if (pin.gpio < 0) { bad(problems, base + "gpio", "required"); continue; }
    claim(pin.gpio, "pin", base + "gpio", true);
    if (!valid_slug(pin.name)) bad(problems, base + "name", pin.name.empty() ? "required" : "invalid");
    else if (std::find(names.begin(), names.end(), pin.name) != names.end()) bad(problems, base + "name", "conflict");
    else names.push_back(pin.name);
    if (pin.mode == PinMode::kAdc && !board::pin_info(pin.gpio).adc1) bad(problems, base + "mode", "no_adc");
    if (!report_allowed(pin.mode, report_of(pin))) bad(problems, base + "report", "invalid");
  }
  validate_perimeter(s.perimeter, problems);
  for (std::size_t z = 0; z < perimeter::kMaxZones; ++z) {   // a zone is a device of the server: its name is not a pin's nor a presence sensor's
    const perimeter::Zone& zone = s.perimeter.zones[z];
    if (zone.enabled && !zone.name.empty() && std::find(names.begin(), names.end(), zone.name) != names.end()) bad(problems, "perimeter.zones." + std::to_string(z) + ".name", "conflict");
  }
  if (std::count_if(s.pins.begin(), s.pins.end(), [](const MappedPin& p) { return p.mode == PinMode::kPwm; }) > 8) bad(problems, "pins", "too_many_pwm");
  // the chip has four PWM timers, one frequency each
  std::vector<int> frequencies;
  for (const MappedPin& pin : s.pins) if (pin.mode == PinMode::kPwm && std::find(frequencies.begin(), frequencies.end(), pin.freq_hz) == frequencies.end()) frequencies.push_back(pin.freq_hz);
  if (frequencies.size() > 4) bad(problems, "pins", "too_many_pwm_freq");
  return problems;
}

// ---- writing -------------------------------------------------------------------------------------------------------------------

inline void write_perimeter(json::Writer& w, const perimeter::Config& c) {
  w.key("perimeter").begin_object().field("enabled", c.enabled).field("range_min_mm", c.range_min_mm).field("range_max_mm", c.range_max_mm).field("debounce_frames", c.debounce_frames)
      .field("count_window", c.count_window).field("strict_entry", c.strict_entry).field("jam_detection", c.jam_detection).field("stream", perimeter::stream_text(c.stream));
  w.key("night").begin_object().field("enabled", c.night_enabled).field("from_min", c.night_from_min).field("to_min", c.night_to_min).end_object();
  w.key("pet").begin_object().field("enabled", c.pet_immunity).field("distance_cm", c.pet_distance_cm).field("energy", c.pet_energy).end_object();
  w.key("masking").begin_object().field("minutes", c.masking_minutes).field("armed_only", c.masking_armed_only).end_object();
  w.key("zones").begin_array();
  for (const perimeter::Zone& z : c.zones) {
    w.begin_object().field("enabled", z.enabled).field("name", z.name).field("exclude", z.exclude).field("x_mm", z.x_mm).field("y_mm", z.y_mm).field("width_mm", z.width_mm).field("height_mm", z.height_mm)
        .field("angle_deg", z.angle_deg).field("profiles", z.profiles).field("latch", z.latch).field("loiter_s", z.loiter_s).field("active", z.active).end_object();
  }
  w.end_array().key("lines").begin_array();
  for (const perimeter::Line& l : c.lines) w.begin_object().field("enabled", l.enabled).field("name", l.name).field("ax_mm", l.ax_mm).field("ay_mm", l.ay_mm).field("bx_mm", l.bx_mm).field("by_mm", l.by_mm).end_object();
  w.end_array().key("paths").begin_array();
  for (const perimeter::Path& q : c.paths) {
    w.begin_object().field("enabled", q.enabled).field("name", q.name).key("zones").begin_array();
    for (const std::string& name : q.zones) if (!name.empty()) w.string(name);
    w.end_array().end_object();
  }
  w.end_array().end_object();
}

// `secrets`: true writes the passwords (for flash storage); false replaces them with "password_set" flags (for the panel).
inline std::string to_json(const Settings& s, bool secrets) {
  json::Writer w;
  w.begin_object();
  w.field("v", kVersion);
  w.key("node").begin_object().field("id", s.node_id).field("name", s.node_name).end_object();
  w.field("uplink", to_text(s.uplink));
  w.key("ip").begin_object().field("dhcp", s.ip.dhcp).field("address", s.ip.address).field("netmask", s.ip.netmask).field("gateway", s.ip.gateway)
      .field("dns1", s.ip.dns1).field("dns2", s.ip.dns2).field("hostname", s.ip.hostname).end_object();
  w.key("ap").begin_object().field("enabled", s.ap.enabled).field("ssid", s.ap.ssid).field("security", to_text(s.ap.security));
  if (secrets) w.field("password", s.ap.password); else w.field("password_set", !s.ap.password.empty());
  w.field("channel", s.ap.channel).field("hidden", s.ap.hidden).field("max_clients", s.ap.max_clients).field("tx_power_dbm", s.ap.tx_power_dbm)
      .field("bandwidth_mhz", s.ap.bandwidth_mhz).field("country", s.ap.country).field("bridge", s.ap.bridge).end_object();
  w.key("sta").begin_object().field("enabled", s.sta.enabled).field("ssid", s.sta.ssid);
  if (secrets) w.field("password", s.sta.password); else w.field("password_set", !s.sta.password.empty());
  w.key("backup").begin_array();
  for (const Network& network : s.sta.backup) {
    w.begin_object().field("ssid", network.ssid);
    if (secrets) w.field("password", network.password); else w.field("password_set", !network.password.empty());
    w.end_object();
  }
  w.end_array();
  w.end_object();
  w.key("mqtt").begin_object().field("enabled", s.mqtt.enabled).field("uri", s.mqtt.uri).field("username", s.mqtt.username);
  if (secrets) w.field("password", s.mqtt.password); else w.field("password_set", !s.mqtt.password.empty());
  w.field("heartbeat_s", s.mqtt.heartbeat_s).field("telemetry_ms", s.mqtt.telemetry_ms).field("tls", s.mqtt.tls).field("ca_pem", s.mqtt.ca_pem);
  w.key("backup").begin_array();
  for (const Broker& broker : s.mqtt.backup) {
    w.begin_object().field("uri", broker.uri).field("username", broker.username);
    if (secrets) w.field("password", broker.password); else w.field("password_set", !broker.password.empty());
    w.end_object();
  }
  w.end_array();
  w.end_object();
  w.key("radars").begin_array();
  for (const RadarLine& radar : s.radars) {
    w.begin_object().field("enabled", radar.enabled).field("model", radar.model).field("baud", radar.baud).field("name", radar.name).field("rx", radar.rx).field("tx", radar.tx)
        .field("offset_x_mm", radar.offset_x_mm).field("offset_y_mm", radar.offset_y_mm).field("yaw_deg", radar.yaw_deg).field("pitch_deg", radar.pitch_deg).field("target_mode", radar.target_mode)
        .field("gate_min", radar.gate_min).field("gate_max", radar.gate_max).field("unmanned_s", radar.unmanned_s);
    w.key("zones").begin_object().field("type", radar.zone_type).key("list").begin_array();
    for (const ZoneRect& rect : radar.zones) w.begin_object().field("x1", rect.x1).field("y1", rect.y1).field("x2", rect.x2).field("y2", rect.y2).end_object();
    w.end_array().end_object().end_object();
  }
  w.end_array();
  w.key("sensors").begin_object().field("veml7700", s.sensors.veml7700).field("sda", s.sensors.sda).field("scl", s.sensors.scl).field("lux_fallback", s.sensors.lux_fallback).end_object();
  w.key("pins").begin_array();
  for (const MappedPin& pin : s.pins) {
    w.begin_object().field("gpio", pin.gpio).field("name", pin.name).field("mode", to_text(pin.mode)).field("invert", pin.invert).field("pull", to_text(pin.pull))
        .field("initial_on", pin.initial_on).field("safe", to_text(pin.safe)).field("link_timeout_s", pin.link_timeout_s).field("pulse_ms", pin.pulse_ms)
        .field("debounce_ms", pin.debounce_ms).field("period_s", pin.period_s).field("freq_hz", pin.freq_hz).field("report", report_of(pin)).field("scale", pin.scale)
        .field("offset", pin.offset).end_object();
  }
  w.end_array();
  w.key("ble").begin_object().field("mode", to_text(s.ble)).end_object();
  w.key("web").begin_object().field("mode", to_text(s.web)).field("admin_sources", s.admin_sources).end_object();
  w.key("ui").begin_object().field("language", s.language).end_object();
  w.key("time").begin_object().field("ntp_enabled", s.time.ntp_enabled).field("ntp", s.time.ntp).field("zone", s.time.zone).end_object();
  w.key("system").begin_object().field("auto_restart_hours", s.auto_restart_hours).field("login_limit_per_minute", s.login_limit_per_minute).field("reader_idle_ms", s.reader_idle_ms).end_object();
  w.key("security").begin_object().field("seal_secrets", s.seal_secrets).end_object();
  w.key("fusion").begin_object().field("merge_mm", s.fusion_merge_mm).end_object();
  write_perimeter(w, s.perimeter);
  w.key("sensor_security").begin_object().field("bluetooth_off", s.sensor_bluetooth_off).end_object();
  w.end_object();
  return w.str();
}

// Reads a stored or received document on top of `base` and checks the result. Nothing is applied unless `problems` stays empty.
inline bool load_document(const json::Value& document, const Settings& base, Settings& out, Problems& problems) {
  out = base;
  read_settings(document, out, problems);
  if (problems.empty()) problems = validate(out);
  return problems.empty();
}

inline bool load(std::string_view text, const Settings& base, Settings& out, Problems& problems) {
  json::Value document;
  if (!json::parse(text, document)) { problems.push_back({"", "not_json"}); return false; }
  return load_document(document, base, out, problems);
}

// What changed in a way that needs a restart: everything but the time zone (applied at once), the node's display name and the panel language.
// The copies are on the heap: whoever saves (the web server, the Bluetooth worker, the serial set-up) has a small stack and a Settings is about a kilobyte.
inline bool needs_restart(const Settings& before, const Settings& after) {
  const std::unique_ptr<Settings> a(new (std::nothrow) Settings(before)), b(new (std::nothrow) Settings(after));
  if (!a || !b) return true;
  a->node_name = b->node_name = "";
  a->language = b->language = "en";
  a->time.zone = b->time.zone = "";
  a->perimeter = b->perimeter = perimeter::Config{};   // the perimeter engine and the rule for the sensors' Bluetooth apply at once
  a->sensor_bluetooth_off = b->sensor_bluetooth_off = true;
  return to_json(*a, true) != to_json(*b, true);
}

}  // namespace armor::config
