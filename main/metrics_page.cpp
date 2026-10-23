// ARMOR-NODE-RADAR - the node's numbers for Prometheus (see metrics_page.hpp). Nothing here has run on a board.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "metrics_page.hpp"

#include <cmath>
extern "C" {
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
}
#include "chip_temp.hpp"
#include "core/metrics.hpp"
#include "mqtt_link.hpp"
#include "network.hpp"
#include "perimeter_rt.hpp"
#include "psram_mem.hpp"
#include "radar_manager.hpp"
#include "sensor_bridge.hpp"

namespace armor::metrics_page {

std::string text() {
  metrics::Page page;
  page.family("armor_firmware_info", "The firmware this node runs", "gauge");
  page.add(1, {{"version", esp_app_get_description()->version}});
  page.gauge("armor_uptime_seconds", "Seconds since the node started", static_cast<double>(esp_timer_get_time() / 1000000LL));
  page.gauge("armor_heap_free_bytes", "Free heap, the external RAM included", static_cast<double>(esp_get_free_heap_size()));
  page.gauge("armor_heap_min_free_bytes", "The least free heap since the start", static_cast<double>(esp_get_minimum_free_heap_size()));
  const membudget::Snapshot memory = memory::snapshot();
  page.gauge("armor_heap_internal_free_bytes", "Free internal memory", static_cast<double>(memory.internal_free));
  page.gauge("armor_heap_internal_min_free_bytes", "The least free internal memory since the start", static_cast<double>(memory.internal_min));
  page.gauge("armor_heap_internal_largest_block_bytes", "The biggest block of internal memory that can be allocated", static_cast<double>(memory.internal_largest));
  page.gauge("armor_psram_free_bytes", "Free external RAM", static_cast<double>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
  page.gauge("armor_chip_temperature_celsius", "Temperature of the chip", chip::temperature_c());   // left out when the sensor cannot be read

  const network::Status net = network::status();
  page.gauge("armor_link_up", "The network link is up and has an address", net.has_ip ? 1 : 0);
  if (net.sta_connected) page.gauge("armor_wifi_rssi_dbm", "Signal strength of the Wi-Fi network the node uses", net.sta_rssi);
  const mqtt_link::Status mqtt = mqtt_link::status();
  page.gauge("armor_mqtt_connected", "The node is connected to the broker", mqtt.connected ? 1 : 0);
  page.counter("armor_mqtt_published_total", "Messages handed to the broker", mqtt.published);

  page.family("armor_sensor_frames_total", "Valid frames read from a sensor", "counter");
  for (std::size_t i = 0; i < 3; ++i) { const radar::Status s = radar::status(i); if (s.enabled) page.add(s.frames, {{"sensor", std::to_string(i + 1)}, {"model", s.model}}); }
  page.family("armor_sensor_bad_frames_total", "Frames that failed their checks", "counter");
  for (std::size_t i = 0; i < 3; ++i) { const radar::Status s = radar::status(i); if (s.enabled) page.add(s.bad_frames, {{"sensor", std::to_string(i + 1)}, {"model", s.model}}); }
  page.family("armor_sensor_frames_per_second", "Frames per second over the last few seconds", "gauge");
  for (std::size_t i = 0; i < 3; ++i) { const radar::Status s = radar::status(i); if (s.enabled) page.add(s.frames_per_second, {{"sensor", std::to_string(i + 1)}, {"model", s.model}}); }
  page.family("armor_sensor_reporting", "1 while the sensor sends valid frames", "gauge");
  for (std::size_t i = 0; i < 3; ++i) { const radar::Status s = radar::status(i); if (s.enabled) page.add(std::string(s.state) == "reporting" ? 1 : 0, {{"sensor", std::to_string(i + 1)}, {"model", s.model}, {"state", s.state}}); }
  page.family("armor_sensor_bluetooth_open", "1 while the sensor's own Bluetooth radio is open", "gauge");
  for (std::size_t i = 0; i < 3; ++i) { const radar::Status s = radar::status(i); if (s.enabled && s.bluetooth != "none") page.add(s.bluetooth == "open" ? 1 : 0, {{"sensor", std::to_string(i + 1)}, {"model", s.model}}); }
  page.family("armor_sensor_pet_ignored_total", "Readings the pet immunity turned into nobody", "counter");
  for (std::size_t i = 0; i < 3; ++i) { const radar::Status s = radar::status(i); if (s.enabled && !s.tracker) page.add(s.pet_ignored, {{"sensor", std::to_string(i + 1)}, {"model", s.model}}); }

  const perimeter_rt::Summary p = perimeter_rt::summary();
  page.gauge("armor_perimeter_enabled", "The node's perimeter engine is on", p.enabled ? 1 : 0);
  page.gauge("armor_targets", "Targets seen now (the median of a few frames)", static_cast<double>(p.targets));
  page.gauge("armor_targets_raw", "Targets seen in the last frame", static_cast<double>(p.raw_targets));
  page.gauge("armor_perimeter_masked", "The sensors saw nothing for the configured time while armed", p.masked ? 1 : 0);
  page.gauge("armor_wifi_jam", "The Wi-Fi signal collapsed the way a jammer makes it", p.wifi_jam ? 1 : 0);
  page.counter("armor_events_sent_total", "Events handed to the broker", p.sent);
  page.counter("armor_events_replayed_total", "Events sent after they waited in flash for the broker", p.replayed);
  page.gauge("armor_events_buffered", "Events waiting in flash for the broker", p.buffered);
  page.counter("armor_events_limited_total", "Events not sent because too many came in a minute", p.limited);
  page.counter("armor_events_dropped_total", "Events lost", p.dropped);
  page.counter("armor_events_flash_failures_total", "Writes of the event ring to flash that failed", p.flash_failures);
  page.gauge("armor_bridge_open", "A serial bridge session is open", sensor_bridge::status().active ? 1 : 0);

  std::vector<perimeter_rt::ZoneMetric> zones;
  std::vector<perimeter_rt::LineMetric> lines;
  std::size_t movement[3];
  perimeter_rt::metrics(zones, lines, movement);
  page.family("armor_targets_by_movement", "Targets by speed class", "gauge");
  page.add(static_cast<double>(movement[0]), {{"class", "still"}});
  page.add(static_cast<double>(movement[1]), {{"class", "walking"}});
  page.add(static_cast<double>(movement[2]), {{"class", "running"}});
  page.family("armor_zone_occupied", "1 while somebody is in the zone", "gauge");
  for (const auto& z : zones) if (!z.exclude) page.add(z.occupied ? 1 : 0, {{"zone", z.name}});
  page.family("armor_zone_on", "1 while the zone is switched on and in its day or night profile", "gauge");
  for (const auto& z : zones) page.add(z.on ? 1 : 0, {{"zone", z.name}, {"kind", z.exclude ? "exclusion" : "detection"}});
  page.family("armor_zone_entries_total", "Times the zone became occupied", "counter");
  for (const auto& z : zones) if (!z.exclude) page.add(z.entries, {{"zone", z.name}});
  page.family("armor_line_crossings_total", "Targets that crossed a tripwire line", "counter");
  for (const auto& l : lines) {
    page.add(l.ins, {{"line", l.name}, {"direction", "in"}});
    page.add(l.outs, {{"line", l.name}, {"direction", "out"}});
  }
  return page.str();
}

}  // namespace armor::metrics_page
