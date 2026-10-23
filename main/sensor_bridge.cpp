// ARMOR-NODE-RADAR - the transparent serial bridge to one sensor (see sensor_bridge.hpp). Nothing here has run on a board; the rules are in core/serial_bridge.hpp and are tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "sensor_bridge.hpp"

#include <mutex>
extern "C" {
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}
#include "core/radar_event.hpp"
#include "core/sensor_model.hpp"
#include "core/serial_bridge.hpp"
#include "entropy.hpp"
#include "perimeter_rt.hpp"
#include "radar_manager.hpp"

namespace armor::sensor_bridge {
namespace {
constexpr char kTag[] = "armor-bridge";
std::mutex g_lock;
bridge::Session g_session;

std::uint64_t monotonic_ms() { return static_cast<std::uint64_t>(esp_timer_get_time()) / 1000ULL; }

std::string model_of(std::size_t sensor) { return radar::status(sensor - 1).model; }

// Ends the session if its time is up, giving the port back. The caller holds g_lock.
void close_if_expired(std::uint64_t now) {
  if (!g_session.active() || now < g_session.expires_at_ms()) return;
  const std::size_t sensor = g_session.sensor();
  const std::string by = g_session.opened_by();
  g_session.close(bridge::End::kExpired);
  radar::bridge_end(sensor - 1);
  ESP_LOGW(kTag, "sensor %u: the bridge opened by \"%s\" expired and was closed", static_cast<unsigned>(sensor), by.c_str());
  perimeter_rt::emit(static_cast<int>(sensor), radarevent::Kind::kBridgeClosed, model_of(sensor), by);
}

void expiry_task(void*) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    std::lock_guard<std::mutex> guard(g_lock);
    close_if_expired(monotonic_ms());
  }
}
}  // namespace

void start() { xTaskCreate(expiry_task, "bridge-exp", 3072, nullptr, 2, nullptr); }

Opened open(std::size_t sensor, std::uint32_t baud, unsigned minutes, const std::string& user) {
  Opened result;
  std::lock_guard<std::mutex> guard(g_lock);
  const std::uint64_t now = monotonic_ms();
  close_if_expired(now);
  if (sensor < 1 || sensor > 3) { result.error = bridge::refusal_text(bridge::Refusal::kBadSensor); return result; }
  const radar::Status line = radar::status(sensor - 1);
  if (baud == 0) {
    sensors::Model model;
    baud = sensors::from_text(line.model, model) ? sensors::info(model).default_baud : 0;
  }
  if (minutes == 0) minutes = bridge::kDefaultMinutes;
  std::uint8_t secret[bridge::kTokenBytes];
  armor::random_bytes(secret, sizeof secret);
  std::string token;
  const bridge::Refusal refusal = g_session.open(sensor, baud, minutes, user, now, secret, token);
  if (refusal != bridge::Refusal::kNone) { result.error = bridge::refusal_text(refusal); return result; }
  if (!line.enabled || !radar::bridge_begin(sensor - 1, baud)) {   // the sensor is not there, its TX wire is not connected (the tool could not write), or a command is running
    g_session.close();
    result.error = "bridge_unavailable";
    return result;
  }
  ESP_LOGW(kTag, "sensor %u: the bridge was opened by \"%s\" at %u baud for %u minutes", static_cast<unsigned>(sensor), user.c_str(), static_cast<unsigned>(baud), minutes);
  perimeter_rt::emit(static_cast<int>(sensor), radarevent::Kind::kBridgeOpened, line.model, user);
  result.ok = true;
  result.token = token;
  result.sensor = static_cast<int>(sensor);
  result.baud = baud;
  result.minutes = minutes;
  return result;
}

Exchanged io(const std::string& token, const std::uint8_t* write, std::size_t write_length, std::size_t want_read, unsigned wait_ms) {
  Exchanged result;
  std::lock_guard<std::mutex> guard(g_lock);   // one exchange at a time, and the port is not given back in the middle of one
  const std::uint64_t now = monotonic_ms();
  close_if_expired(now);
  const bridge::Refusal refusal = g_session.exchange(token, write_length, want_read, now);
  if (refusal != bridge::Refusal::kNone) { result.error = bridge::refusal_text(refusal); return result; }
  if (wait_ms > 500) wait_ms = 500;
  result.bytes.assign(want_read, '\0');
  const std::size_t got = want_read == 0 ? (radar::bridge_exchange(g_session.sensor() - 1, write, write_length, nullptr, 0, 0), 0)
                                         : radar::bridge_exchange(g_session.sensor() - 1, write, write_length, reinterpret_cast<std::uint8_t*>(&result.bytes[0]), want_read, wait_ms);
  result.bytes.resize(got);
  g_session.note_read(got, want_read);
  result.ok = true;
  return result;
}

bool close(const std::string& user) {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_session.active()) return false;
  const std::size_t sensor = g_session.sensor();
  const std::string by = g_session.opened_by();
  g_session.close(bridge::End::kStopped);
  radar::bridge_end(sensor - 1);
  ESP_LOGW(kTag, "sensor %u: the bridge opened by \"%s\" was closed by \"%s\"", static_cast<unsigned>(sensor), by.c_str(), user.c_str());
  perimeter_rt::emit(static_cast<int>(sensor), radarevent::Kind::kBridgeClosed, model_of(sensor), user);
  return true;
}

Status status() {
  Status s;
  std::lock_guard<std::mutex> guard(g_lock);
  close_if_expired(monotonic_ms());
  s.active = g_session.active();
  if (!s.active) return s;
  s.sensor = static_cast<int>(g_session.sensor());
  s.baud = g_session.baud();
  const std::uint64_t now = monotonic_ms();
  s.seconds_left = g_session.expires_at_ms() > now ? static_cast<unsigned>((g_session.expires_at_ms() - now) / 1000) : 0;
  s.by = g_session.opened_by();
  s.to_sensor = g_session.bytes_to_sensor();
  s.from_sensor = g_session.bytes_from_sensor();
  return s;
}

}  // namespace armor::sensor_bridge
