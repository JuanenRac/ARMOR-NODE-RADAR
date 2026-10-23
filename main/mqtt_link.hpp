// ARMOR-NODE-RADAR - the node's link to the broker: the clock, the health message, the radar telemetry and the mapped pins' topics.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstdint>
#include <functional>
#include <string>

#include "core/node_config.hpp"

namespace armor::mqtt_link {

// Starts the task that waits for an address, sets the clock (SNTP) and connects to the broker of the settings. Does nothing when the
// settings have no broker (mqtt disabled or no address yet): the node then only serves its panel.
void start(const config::Settings& settings);

using MessageHandler = std::function<void(const std::string& topic, const std::string& payload)>;
// Asks for the messages of one topic (an exact topic, no wildcards). Call it before start(): on every (re)connection of the broker the topic is subscribed again, and every COMPLETE message
// on it goes to `on_message` in the broker client's own task, so the handler must not wait for long.
void subscribe(const std::string& topic, MessageHandler on_message);

bool connected();
bool clock_is_set();
std::uint64_t wall_clock_ms();

void publish(const std::string& topic, const std::string& payload);

// Publishes one message retained, at QoS 1: only the planned-maintenance signal (armor/control/<node>/maintenance) is sent so; everything else stays QoS 0 and not retained.
void publish_retained(const std::string& topic, const std::string& payload);

// Hands one message to the broker with delivery guaranteed at least once (QoS 1) and says whether the client took it: false when the broker is not connected. The events of the perimeter engine go this way;
// the caller keeps what was refused (core/offline_ring.hpp) and sends it again.
bool publish_event(const std::string& topic, const std::string& payload);

struct Status {
  bool enabled = false;
  bool connected = false;
  bool clock_set = false;
  std::uint32_t published = 0;
  std::string withheld;   // "" or why telemetry is not being sent ("light", "radars")
};
Status status();

}  // namespace armor::mqtt_link
