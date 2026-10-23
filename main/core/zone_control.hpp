// ARMOR-NODE-RADAR - the orders a zone of the perimeter engine takes from the server, and the armed state. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// A zone of the engine is a device of the server like a mapped output is: it reports `{"on":true}` on armor/device/<node>/<zone>/state and takes its orders on armor/device/<node>/<zone>/set, in the
// words a pin takes (on, off, toggle, true, false, 1, 0, or {"on":true}). The server's own devices and automations therefore switch it with nothing new: a window contact that opens turns an
// exclusion zone off for as long as it is open. One more word, `release`, lets go of a zone that holds a target until it is seen leaving. The zone names share the namespace of the pins and of the
// presence sensors (the settings refuse a name that is already used).
//
// Whether the perimeter is armed is told on armor/radar/<node>/control/armed (on or off, also armed/disarmed). It only matters to the rule that suspects a covered sensor.
#pragma once
#include <string>
#include <string_view>

#include "gpio_logic.hpp"
#include "node_id.hpp"

namespace armor::zonecontrol {

enum class Order : std::uint8_t { kNone, kOn, kOff, kToggle, kRelease };

inline Order parse(std::string_view payload) {
  const std::string word = gpio::lower_trimmed(payload);
  if (word == "release") return Order::kRelease;
  switch (gpio::parse_command(payload, config::PinMode::kOutput).action) {
    case gpio::Action::kOn: return Order::kOn;
    case gpio::Action::kOff: return Order::kOff;
    case gpio::Action::kToggle: return Order::kToggle;
    default: return Order::kNone;   // a pulse or a level means nothing to a zone
  }
}

inline std::string armed_topic(std::string_view node_id) { return node_id_is_valid(node_id) ? "armor/radar/" + std::string(node_id) + "/control/armed" : ""; }

// The payload of the armed topic: 1 armed, 0 disarmed, -1 not understood.
inline int parse_armed(std::string_view payload) {
  const std::string word = gpio::lower_trimmed(payload);
  if (word == "on" || word == "armed" || word == "true" || word == "1") return 1;
  if (word == "off" || word == "disarmed" || word == "false" || word == "0") return 0;
  return -1;
}

inline std::string state_payload(bool on) { return gpio::report_boolean("on", on); }

}  // namespace armor::zonecontrol
