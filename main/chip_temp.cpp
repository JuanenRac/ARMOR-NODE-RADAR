// ARMOR-NODE-RADAR - the temperature of the chip (see chip_temp.hpp). Nothing here has run on a board.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "chip_temp.hpp"

#include <cmath>
#include <mutex>
extern "C" {
#include "driver/temperature_sensor.h"
}

namespace armor::chip {
namespace {
std::mutex g_lock;
temperature_sensor_handle_t g_handle = nullptr;
bool g_tried = false;
}  // namespace

float temperature_c() {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_tried) {
    g_tried = true;
    temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    if (temperature_sensor_install(&config, &g_handle) != ESP_OK || temperature_sensor_enable(g_handle) != ESP_OK) g_handle = nullptr;
  }
  if (g_handle == nullptr) return NAN;
  float celsius = 0.0f;
  if (temperature_sensor_get_celsius(g_handle, &celsius) != ESP_OK) return NAN;
  return celsius;
}

}  // namespace armor::chip
