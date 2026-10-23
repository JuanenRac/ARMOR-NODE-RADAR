/*
 * ARMOR-NODE-RADAR - ESP32-S3 field node (Waveshare ESP32-S3-ETH, up to three HLK-LD2450 radars, wired Ethernet and a Wi-Fi access point).
 * Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
 *
 * The hardware-independent core under main/core is built and tested on a computer (tests/); the files beside this one are built with
 * ESP-IDF 5.4 (tools/build_node.sh does it in a container). It has run on real boards (a Waveshare ESP32-S3-ETH with a radar, reporting to the bench server); the
 * LD2461 and the presence sensors have not been tried with hardware: the bench checklist is in docs/BENCH_BRINGUP.md.
 *
 * What a node is, in one paragraph: everything that differs from node to node (its identity, its address, its Wi-Fi network, its broker,
 * its radars and the pins it lends to the server) is one settings document in flash, edited from the node's own web panel, so the same
 * firmware image serves every node and no password is compiled in. A node with no user yet is in setup: it opens the network
 * "ARMOR-SETUP-xxxxxx" and shows a setup code on its USB console. The radars are read, their frames checked and their health counted;
 * telemetry goes to the broker only when the network, the clock, a light value and at least one radar that is really reporting are all
 * there, and never as an empty "all clear" from radars that are silent. The firmware can be replaced from the panel; a new image that
 * does not come up is rolled back by the boot loader.
 */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
extern "C" {
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
}
#include "api_shared.hpp"
#include "ble_provision.hpp"
#include "clock_sync.hpp"
#include "improv_serial.hpp"
#include "core/network_plan.hpp"
#include "core/radar_report.hpp"
#include "gpio_manager.hpp"
#include "light_sensor.hpp"
#include "log_buffer.hpp"
#include "psram_mem.hpp"
#include "mqtt_link.hpp"
#include "node_health.hpp"
#include "node_control.hpp"
#include "network.hpp"
#include "node_store.hpp"
#include "perimeter_rt.hpp"
#include "radar_manager.hpp"
#include "sensor_bridge.hpp"
#include "web_server.hpp"

namespace {
constexpr char kTag[] = "armor-node";

// While the node has no user, says every 15 seconds how to set it up. The code appears only here, on the USB console.
void setup_reminder_task(void*) {
  for (;;) {
    if (!armor::store::users_empty()) vTaskDelete(nullptr);
    const armor::network::Status n = armor::network::status();
    ESP_LOGW(kTag, "SETUP: this node has no user yet. Open http://%s/ (or join the Wi-Fi \"%s\" and open http://192.168.4.1/) and enter the setup code %s (to pair over Bluetooth the phone asks for the PIN %06u)",
             n.has_ip ? n.ip.c_str() : "<its address>", n.ap_ssid.empty() ? "ARMOR-SETUP-..." : n.ap_ssid.c_str(), armor::store::setup_code().c_str(), static_cast<unsigned>(armor::auth::ble_passkey_from(armor::store::setup_code())));
    vTaskDelay(pdMS_TO_TICKS(15000));
  }
}

#if CONFIG_ARMOR_BOOT_FACTORY_RESET
// Holding the BOOT button for 8 seconds in the first 30 after power-up erases the settings and the users: the node is back in setup.
// (Skipped when the operator mapped GPIO 0 as one of the node's own pins.)
void boot_button_task(void*) {
  constexpr gpio_num_t kButton = GPIO_NUM_0;
  gpio_config_t io{};
  io.pin_bit_mask = 1ULL << kButton;
  io.mode = GPIO_MODE_INPUT;
  io.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io);
  int held_ms = 0;
  for (int elapsed_ms = 0; elapsed_ms < 30000; elapsed_ms += 100) {
    held_ms = gpio_get_level(kButton) == 0 ? held_ms + 100 : 0;
    if (held_ms >= 8000) {
      ESP_LOGW(kTag, "BOOT held for 8 s: erasing the settings and the users");
      armor::store::factory_reset();
      armor::network::disconnect_before_restart();
      esp_restart();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  vTaskDelete(nullptr);
}
#endif

// A freshly installed firmware is on probation: once the panel is up and has stayed up for a while, it is confirmed; if the panel could
// not start, the boot loader is told to go back to the previous image now.
void confirm_firmware_task(void* argument) {
  const bool panel_ok = argument != nullptr;
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
    if (!panel_ok) {
      ESP_LOGE(kTag, "the new firmware could not start its panel: going back to the previous one");
      esp_ota_mark_app_invalid_rollback_and_reboot();
    }
    vTaskDelay(pdMS_TO_TICKS(30000));
    // A panel that is up is not enough: a node that cannot be reached (no address from the network, no access point of its own) would stay that way and need a cable to be rescued. The image
    // is confirmed once the node can be reached, and goes back to the previous one when five minutes pass without.
    bool reachable = false;
    for (int waited = 0; waited < 300 && !reachable; waited += 5) {
      const armor::network::Status link = armor::network::status();
      reachable = link.has_ip || link.ap_active;
      if (!reachable) vTaskDelay(pdMS_TO_TICKS(5000));
    }
    if (!reachable) {
      ESP_LOGE(kTag, "the new firmware could not be reached on any network: going back to the previous one");
      esp_ota_mark_app_invalid_rollback_and_reboot();
    }
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(kTag, "the new firmware is confirmed");
  }
  vTaskDelete(nullptr);
}
}  // namespace

extern "C" void app_main() {
  armor::logbuf::start();
  armor::memory::install_tls_allocator();   // TLS memory: the external RAM, and the internal one where a board has none
  armor::memory::start_monitor();           // the log says the state of the memory every minute
  armor::store::init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  const armor::config::Settings settings = armor::store::settings();
  const bool setup = armor::store::users_empty();
  ESP_LOGI(kTag, "A.R.M.O.R. node %s, firmware %s, MAC tail %s%s", settings.node_id.c_str(), esp_app_get_description()->version, armor::store::mac_tail().c_str(), setup ? ", NOT SET UP YET" : "");
  const armor::config::Problems problems = armor::config::validate(settings);
  for (const armor::config::Problem& problem : problems) ESP_LOGE(kTag, "settings problem: %s (%s)", problem.path.c_str(), problem.code.c_str());

  // What the server can order (armor/control/<node>/...: restart, pause, standby, identify, diagnostics, maintenance; two steps with a one-time token the node gives, never a factory reset). A node
  // that was put in standby boots straight into it, so this comes BEFORE the radars start: while the node is paused or in standby it publishes no targets and no presence. It listens from the
  // broker's first connection, so it is set up before the link starts.
  if (!setup && settings.mqtt.enabled) {
    armor::node_control::Options control;
    control.node_id = settings.node_id;
    control.publish = [](const std::string& topic, const std::string& payload) { armor::mqtt_link::publish(topic, payload); };
    control.publish_retained = [](const std::string& topic, const std::string& payload) { armor::mqtt_link::publish_retained(topic, payload); };
    // The firmware of each sensor goes into the diagnostic package (name and version only, from the identity the node read from the sensor).
    control.sensor_versions = []() {
      std::vector<armor::diag::Sensor> out;
      for (const armor::radarreport::Sensor& sensor : armor::radar::report_sensors()) {
        if (sensor.firmware_known && !sensor.firmware.empty()) out.push_back({sensor.model + "-" + std::to_string(sensor.sensor_id), sensor.firmware});
      }
      return out;
    };
    armor::node_control::start(control);
  }
  // The radars run first: on the bench their statistics are the first thing to look at, with or without a network.
  armor::radar::start(settings, [](const std::string& topic, const std::string& payload) { armor::mqtt_link::publish(topic, payload); });
  if (!armor::light_sensor_start(settings.sensors)) ESP_LOGE(kTag, "the light sensor could not be started: telemetry stays withheld unless a light fallback is set");
  armor::pins::start(
      settings, [](const std::string& topic, const std::string& payload) { armor::mqtt_link::publish(topic, payload); },
      []() { return !armor::mqtt_link::status().enabled || armor::mqtt_link::connected(); });

  const armor::netplan::Plan plan = armor::netplan::plan_network(settings, setup, armor::store::setup_code(), armor::store::mac_tail(), armor::store::mac_sum());
  const bool network_ok = armor::network::start(settings, plan);
  if (!network_ok) ESP_LOGE(kTag, "the network could not be started: the node stays local");
  if (network_ok && !plan.ap.enabled) {
    // Without an address after 90 s (no cable, no Wi-Fi it can join) the node would be unreachable: it opens the set-up access point again.
    armor::network::arm_rescue(armor::netplan::plan_network(settings, true, armor::store::setup_code(), armor::store::mac_tail(), armor::store::mac_sum()), 90);
  }
  armor::clocksync::start(settings);
  armor::improv_serial::start();   // Improv Wi-Fi over the USB cable (ESP Web Tools)
  const bool panel_ok = network_ok && armor::web::start(settings);
  armor::mqtt_link::start(settings);
  // The node's own perimeter engine and the events it tells the server (zones, tripwire lines, paths, a covered sensor, a sensor whose own Bluetooth is open...), kept in flash while the broker cannot be reached.
  {
    armor::perimeter_rt::Hooks hooks;
    hooks.connected = []() { return armor::mqtt_link::connected(); };
    hooks.send = [](const std::string& topic, const std::string& payload) { return armor::mqtt_link::publish_event(topic, payload); };
    hooks.publish = [](const std::string& topic, const std::string& payload) { armor::mqtt_link::publish(topic, payload); };
    hooks.wall_ms = []() { return armor::mqtt_link::wall_clock_ms(); };
    armor::perimeter_rt::start(settings, hooks);
  }
  armor::sensor_bridge::start();
  // The health report every node says about itself (uptime, why it restarted, memory, link, broker reconnects, the last error): armor/health/<node>, see ARMOR-COMMON node_health.schema.json.
  if (!setup && settings.mqtt.enabled) {
    armor::node_health::Options health;
    health.node_id = settings.node_id;
    health.family = "radar";
    // the report of the shared base, with the sensors (who they are, their firmware, their Bluetooth) and the perimeter engine added to it
    health.publish = [](const std::string& topic, const std::string& payload) { armor::mqtt_link::publish(topic, armor::radarreport::splice(payload, armor::api::health_members_json())); };
    health.connected = []() { return armor::mqtt_link::connected(); };
    armor::node_health::start(health);
  }

  // A periodic, unconditional restart the panel can turn on (0: never) - a board left running for weeks without anyone touching it.
  if (settings.auto_restart_hours > 0) armor::web::restart_after(static_cast<unsigned>(settings.auto_restart_hours) * 3600u * 1000u);

  if (setup) xTaskCreate(setup_reminder_task, "setup-hint", 3072, nullptr, 2, nullptr);
#if CONFIG_ARMOR_BOOT_FACTORY_RESET
  bool button_is_a_pin = false;
  for (const armor::config::MappedPin& pin : settings.pins) if (pin.gpio == 0 && pin.mode != armor::config::PinMode::kDisabled) button_is_a_pin = true;
  if (!button_is_a_pin) xTaskCreate(boot_button_task, "boot-button", 3072, nullptr, 2, nullptr);
#endif
  xTaskCreate(confirm_firmware_task, "confirm", 3072, panel_ok ? reinterpret_cast<void*>(1) : nullptr, 2, nullptr);
  // Bluetooth goes last: its controller takes a large share of the internal memory, and every task above (the broker link, the health report, the confirmation of a new image) must already exist.
  armor::ble_provision::start(settings, setup);   // only when the settings say so: Bluetooth stays unused otherwise
}
