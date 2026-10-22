// ARMOR-RADAR - the perimeter engine at run time: it takes the node's targets ten times a second, runs the rules of core/perimeter.hpp and tells what happened to the server (events), keeping the
// events in flash while the broker cannot be reached. It also watches the Wi-Fi signal for jamming, learns static reflectors on request, and answers the orders a zone takes from the server.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "core/node_config.hpp"
#include "core/radar_event.hpp"
#include "core/zone_control.hpp"
#include "radar_tracks.hpp"

namespace armor::perimeter_rt {

struct Hooks {
  std::function<bool()> connected;                                                    // the broker is connected
  std::function<bool(const std::string& topic, const std::string& payload)> send;     // hands one message to the broker (QoS 1); false when it was not accepted
  std::function<void(const std::string& topic, const std::string& payload)> publish;  // an ordinary state message
  std::function<std::uint64_t()> wall_ms;                                             // the time of an event
};

// Starts the task. Does nothing but keep the settings when the engine, the jam detection and the sensors' events all have nothing to do (the task is cheap anyway).
void start(const config::Settings& settings, const Hooks& hooks);

// A saved setting: the engine is made again only when the perimeter part changed; the pet rule and the rule for the sensors' Bluetooth are handed on.
void apply(const config::Settings& settings);

// An event of a sensor (its Bluetooth found open or closed, the serial bridge opened or closed). Safe to call from any task: it is queued and delivered by the perimeter task.
void emit(int sensor_id, radarevent::Kind kind, const std::string& name, const std::string& by = "");

// The broker connected: the events kept in flash are sent, and the zones say their state.
void on_broker_connected();

// An order for a zone (see core/zone_control.hpp). False when no zone has that name.
bool zone_order(const std::string& zone, zonecontrol::Order order);
void set_armed(bool armed);
bool armed();

// The speed class of a target (still, walking or running), for the live map.
const char* movement_of(int sensor_id, int track_id);

// Whether this telemetry message is sent (the setting `stream`): the targets are always the real ones.
bool should_send_targets(const Track* tracks, std::size_t count, std::uint64_t now_ms);

// Learning of static reflectors: a run of `seconds` with the area empty, then suggestions for exclusion zones.
bool learn_start(unsigned seconds);
void learn_stop();

struct Summary {
  bool enabled = false;
  bool masked = false;
  bool wifi_jam = false;
  unsigned buffered = 0;           // events waiting in flash for the broker
  std::uint32_t sent = 0;          // events handed to the broker
  std::uint32_t replayed = 0;      // of those, the ones that waited in flash
  std::uint32_t limited = 0;       // events not sent because too many came in a minute
  std::uint32_t dropped = 0;       // events lost: the ring was full, or the engine could not hold them
  std::uint32_t flash_failures = 0;
  std::size_t targets = 0;         // the number of targets, median of a few frames
  std::size_t raw_targets = 0;
};
Summary summary();

// The panel's view: zones, lines, counts by movement, the learner and its suggestions.
std::string state_json();

// For /metrics: the zones and lines as plain numbers.
struct ZoneMetric { std::string name; bool exclude; bool on; bool occupied; unsigned occupants; std::uint32_t entries; };
struct LineMetric { std::string name; std::uint32_t ins, outs; };
void metrics(std::vector<ZoneMetric>& zones, std::vector<LineMetric>& lines, std::size_t movement[3]);

}  // namespace armor::perimeter_rt
