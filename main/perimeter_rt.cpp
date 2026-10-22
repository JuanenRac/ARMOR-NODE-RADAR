// ARMOR-RADAR - the perimeter engine at run time. What it decides is in core/perimeter.hpp, core/offline_ring.hpp, core/jam_detector.hpp and core/reflector_learner.hpp (all tested on a computer);
// this file feeds them the node's targets and hands their events to the broker.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include "perimeter_rt.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <ctime>
#include <deque>
#include <memory>
#include <mutex>
extern "C" {
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}
#include "core/gpio_logic.hpp"
#include "core/jam_detector.hpp"
#include "core/json.hpp"
#include "core/offline_ring.hpp"
#include "core/perimeter.hpp"
#include "core/radar_calibration.hpp"
#include "core/reflector_learner.hpp"
#include "network.hpp"
#include "node_guard.hpp"
#include "node_store.hpp"
#include "radar_manager.hpp"

namespace armor::perimeter_rt {
namespace {
constexpr char kTag[] = "armor-perimeter";
constexpr std::size_t kQueueMax = 64;
constexpr unsigned kEventsPerMinute = 60;
constexpr std::uint64_t kStatesEveryMs = 60000;

std::uint64_t monotonic_ms() { return static_cast<std::uint64_t>(esp_timer_get_time()) / 1000ULL; }

// The two places of the ring in the settings store: two blobs, the newer one wins (see core/offline_ring.hpp).
class NvsStorage : public offline::Storage {
 public:
  bool read(int place, std::string& out) override { return store::blob_read(place == 0 ? "evring_a" : "evring_b", out); }
  bool write(int place, const std::string& blob) override { return store::blob_write(place == 0 ? "evring_a" : "evring_b", blob); }
};

std::mutex g_lock;                      // the engine, its settings, the learner, the jam detector and the stream gate
Hooks g_hooks;
std::string g_node_id;
std::unique_ptr<perimeter::Engine> g_engine;
std::string g_config_text;              // the perimeter part of the settings as text: a save that did not change it leaves the engine as it is
std::array<config::RadarLine, config::kRadarCount> g_radars{};
int g_merge_mm = 0;
bool g_uplink_wifi = false;
std::unique_ptr<reflectors::Learner> g_learner;
jam::Detector g_jam;
bool g_jam_enabled = false;
bool g_jammed = false;
radarevent::StreamGate g_stream_gate;
perimeter::Stream g_stream = perimeter::Stream::kLive;
bool g_zone_sent[perimeter::kMaxZones]{};
bool g_zone_last[perimeter::kMaxZones]{};
std::uint64_t g_states_at_ms = 0;
std::atomic<bool> g_armed{false};
std::atomic<bool> g_states_due{true};

// Only the perimeter task touches these two (the flash is written from a task whose stack is in the internal memory).
NvsStorage g_storage;
std::unique_ptr<offline::DurableRing> g_ring;
std::unique_ptr<radarevent::Sequence> g_sequence;
unsigned g_tokens = kEventsPerMinute;
std::uint64_t g_tokens_at_ms = 0;
std::uint64_t g_flushed_at_ms = 0;

std::mutex g_queue_lock;                // the events that wait to be delivered
std::deque<radarevent::Event> g_queue;

std::atomic<std::uint32_t> g_sent{0}, g_replayed{0}, g_limited{0}, g_dropped{0};
std::atomic<unsigned> g_buffered{0};
std::atomic<std::uint32_t> g_flash_failures{0};

bool night_now(const perimeter::Config& c) {
  if (!c.night_enabled) return false;
  const std::time_t t = std::time(nullptr);
  if (t < 1700000000) return false;   // the clock is not set: it is day
  std::tm local{};
  localtime_r(&t, &local);
  const int minute = local.tm_hour * 60 + local.tm_min;
  return c.night_from_min <= c.night_to_min ? (minute >= c.night_from_min && minute < c.night_to_min) : (minute >= c.night_from_min || minute < c.night_to_min);
}

void enqueue(const radarevent::Event& event) {
  std::lock_guard<std::mutex> guard(g_queue_lock);
  if (g_queue.size() >= kQueueMax) { ++g_dropped; return; }
  g_queue.push_back(event);
}

std::uint64_t wall_ms() { return g_hooks.wall_ms ? g_hooks.wall_ms() : monotonic_ms(); }

// An event of the engine in the words of the contract.
radarevent::Event to_event(const perimeter::Event& e, const perimeter::Config& cfg, std::uint64_t at_ms) {
  radarevent::Event out;
  out.kind = radarevent::kind_of(e.type);
  out.timestamp_ms = at_ms;
  const auto zone_name = [&](int index) -> std::string { return index >= 0 && index < static_cast<int>(perimeter::kMaxZones) ? cfg.zones[static_cast<std::size_t>(index)].name : std::string(); };
  if (e.sensor >= 1 && e.sensor <= 3 && e.track >= 1) {
    out.sensor_id = e.sensor;
    out.track_id = e.track;
    out.movement = perimeter::movement_text(e.movement);
  }
  switch (e.type) {
    case perimeter::Event::Type::kZoneOccupied: out.name = zone_name(e.index); out.value = e.value; break;
    case perimeter::Event::Type::kZoneClear: out.name = zone_name(e.index); out.value = e.value; break;
    case perimeter::Event::Type::kLoiter: out.name = zone_name(e.index); out.value = e.value; break;
    case perimeter::Event::Type::kTripwire:
      if (e.index >= 0 && e.index < static_cast<int>(perimeter::kMaxLines)) out.name = cfg.lines[static_cast<std::size_t>(e.index)].name;
      out.direction = e.value > 0 ? "in" : "out";
      out.count = e.value2;
      break;
    case perimeter::Event::Type::kPathViolation: out.name = zone_name(e.index); out.from = zone_name(e.value); break;
    case perimeter::Event::Type::kMasking: out.value = e.value; break;
    case perimeter::Event::Type::kMaskingEnd: break;
  }
  return out;
}

// One look at the targets: the engine runs and its events are queued.
void step(std::uint64_t now) {
  Track tracks[armor::kMaximumTracks];
  std::size_t count = radar::collect_tracks(tracks);
  perimeter::Event events[8];
  radarevent::Event converted[8];
  std::size_t n = 0;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    armor::calibration::apply_all(tracks, count, g_radars);
    count = armor::calibration::merge_overlap(tracks, count, g_merge_mm);
    if (g_learner && g_learner->active()) g_learner->feed(tracks, count, now);
    if (!g_engine->config().enabled) return;
    perimeter::Context ctx;
    ctx.now_ms = now;
    ctx.night = night_now(g_engine->config());
    ctx.armed = g_armed;
    n = g_engine->update(tracks, count, ctx, events, 8);
    const std::uint64_t at = wall_ms();
    for (std::size_t i = 0; i < n; ++i) converted[i] = to_event(events[i], g_engine->config(), at);
  }
  for (std::size_t i = 0; i < n; ++i) enqueue(converted[i]);
}

void send_or_buffer(const std::string& topic, const std::string& json) {
  if (g_hooks.connected && g_hooks.connected() && g_ring->size() == 0 && g_hooks.send && g_hooks.send(topic, json)) { ++g_sent; return; }
  if (!g_ring->push(topic, json, false, false)) ++g_dropped;
  g_buffered = static_cast<unsigned>(g_ring->size());
}

void deliver(std::uint64_t now) {
  std::deque<radarevent::Event> batch;
  { std::lock_guard<std::mutex> guard(g_queue_lock); batch.swap(g_queue); }
  const std::string topic = radarevent::topic(g_node_id);
  for (const radarevent::Event& event : batch) {
    // no more than a minute's worth in a minute: a ghost that flaps does not fill the broker or the flash
    const std::uint64_t elapsed = now - g_tokens_at_ms;
    if (elapsed >= 1000) {
      const std::uint64_t gained = elapsed * kEventsPerMinute / 60000ULL;
      if (gained > 0) { g_tokens = static_cast<unsigned>(std::min<std::uint64_t>(kEventsPerMinute, g_tokens + gained)); g_tokens_at_ms = now; }
    }
    if (g_tokens == 0) { ++g_limited; continue; }
    --g_tokens;
    if (topic.empty()) continue;
    send_or_buffer(topic, radarevent::to_json(g_node_id, g_sequence->boot(), g_sequence->next(), event));
  }
}

// The events that waited in flash go out in order as soon as the broker takes them.
void replay() {
  if (g_ring->size() == 0 || !g_hooks.connected || !g_hooks.connected() || !g_hooks.send) return;
  const std::size_t sent = g_ring->drain([&](const offline::Message& message) { return g_hooks.send(message.topic, message.payload); });
  g_sent += static_cast<std::uint32_t>(sent);
  g_replayed += static_cast<std::uint32_t>(sent);
  g_buffered = static_cast<unsigned>(g_ring->size());
  if (sent > 0) ESP_LOGI(kTag, "%u event(s) that waited for the broker were sent; %u still wait", static_cast<unsigned>(sent), static_cast<unsigned>(g_ring->size()));
}

void flush_ring(std::uint64_t now) {
  if (!g_ring->dirty() || now - g_flushed_at_ms < 1000) return;
  g_flushed_at_ms = now;
  const std::uint32_t before = g_ring->failed_writes();
  g_ring->flush();
  g_flash_failures += g_ring->failed_writes() - before;
  g_buffered = static_cast<unsigned>(g_ring->size());
}

// Every zone is a device of the server: it says whether it is on, when that changes and every minute.
void publish_states(std::uint64_t now) {
  if (!g_hooks.connected || !g_hooks.connected() || !g_hooks.publish) return;
  const bool force = g_states_due.exchange(false) || now - g_states_at_ms >= kStatesEveryMs;
  std::vector<std::pair<std::string, std::string>> messages;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    for (std::size_t z = 0; z < perimeter::kMaxZones; ++z) {
      const perimeter::Zone& zone = g_engine->config().zones[z];
      if (!zone.enabled) { g_zone_sent[z] = false; continue; }
      const bool on = g_engine->zone_is_active(z);
      if (force || !g_zone_sent[z] || g_zone_last[z] != on) {
        messages.emplace_back(gpio::topic(g_node_id, zone.name, "state"), zonecontrol::state_payload(on));
        g_zone_sent[z] = true;
        g_zone_last[z] = on;
      }
    }
  }
  if (force) g_states_at_ms = now;
  for (const auto& message : messages) if (!message.first.empty()) g_hooks.publish(message.first, message.second);
}

void look_at_wifi(std::uint64_t now) {
  static std::uint64_t last_ms = 0;
  if (now - last_ms < 5000) return;
  last_ms = now;
  bool enabled, wifi;
  { std::lock_guard<std::mutex> guard(g_lock); enabled = g_jam_enabled; wifi = g_uplink_wifi; }
  if (!enabled || !wifi) return;
  const network::Status net = network::status();
  int change;
  bool jammed;
  {
    std::lock_guard<std::mutex> guard(g_lock);
    change = g_jam.feed(net.sta_connected ? net.sta_rssi : 0, net.sta_connected && net.has_ip);
    g_jammed = g_jam.jammed();
    jammed = g_jammed;
  }
  if (change != 0) {
    ESP_LOGW(kTag, "the Wi-Fi signal %s (%d dBm)", jammed ? "collapsed the way a jammer makes it" : "is back", net.sta_rssi);
    radarevent::Event event;
    event.kind = jammed ? radarevent::Kind::kWifiJam : radarevent::Kind::kWifiJamEnd;
    event.timestamp_ms = wall_ms();
    enqueue(event);
  }
}

void perimeter_task(void*) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(100));
    const std::uint64_t now = monotonic_ms();
    if (radar::any_fresh()) step(now);
    look_at_wifi(now);
    deliver(now);
    replay();
    flush_ring(now);
    publish_states(now);
  }
}
}  // namespace

void start(const config::Settings& settings, const Hooks& hooks) {
  g_hooks = hooks;
  g_node_id = settings.node_id;
  g_engine = std::make_unique<perimeter::Engine>();
  g_ring = std::make_unique<offline::DurableRing>(g_storage);
  const std::size_t waiting = g_ring->load();
  g_buffered = static_cast<unsigned>(waiting);
  if (waiting > 0) ESP_LOGW(kTag, "%u event(s) were waiting for the broker when the node restarted: they are sent when it connects", static_cast<unsigned>(waiting));
  g_sequence = std::make_unique<radarevent::Sequence>((esp_random() & 0x7FFFFFFFu) | 1u);
  apply(settings);
  radar::set_event_sink([](int sensor_id, radarevent::Kind kind, const std::string& name) { emit(sensor_id, kind, name); });
  nodeguard::expect_task("perimeter");
  xTaskCreate(perimeter_task, "perimeter", 6144, nullptr, 3, nullptr);   // the internal memory: it writes the flash (the ring), and a stack in the external RAM cannot while that happens
}

void apply(const config::Settings& settings) {
  json::Writer w;
  w.begin_object();
  config::write_perimeter(w, settings.perimeter);
  w.end_object();
  {
    std::lock_guard<std::mutex> guard(g_lock);
    if (!g_engine) return;
    g_radars = settings.radars;
    g_merge_mm = settings.fusion_merge_mm;
    g_uplink_wifi = settings.uplink == config::Uplink::kWifi;
    g_stream = settings.perimeter.stream;
    g_jam_enabled = settings.perimeter.jam_detection;
    if (w.str() != g_config_text) {
      g_config_text = w.str();
      g_engine->configure(settings.perimeter);
      ESP_LOGI(kTag, "perimeter engine %s: %u zone(s) on, %s", settings.perimeter.enabled ? "on" : "off", static_cast<unsigned>(std::count_if(settings.perimeter.zones.begin(), settings.perimeter.zones.end(), [](const perimeter::Zone& z) { return z.enabled; })),
               perimeter::stream_text(settings.perimeter.stream));
    }
  }
  pet::Rule rule;
  rule.enabled = settings.perimeter.pet_immunity;
  rule.max_distance_cm = settings.perimeter.pet_distance_cm;
  rule.max_energy = settings.perimeter.pet_energy;
  radar::set_pet_rule(rule);
  radar::set_close_bluetooth(settings.sensor_bluetooth_off);
  g_states_due = true;
}

void emit(int sensor_id, radarevent::Kind kind, const std::string& name, const std::string& by) {
  radarevent::Event event;
  event.kind = kind;
  event.timestamp_ms = wall_ms();
  event.name = name;
  event.sensor_id = sensor_id;
  event.by = by;
  enqueue(event);
}

void on_broker_connected() { g_states_due = true; }

bool zone_order(const std::string& zone, zonecontrol::Order order) {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_engine) return false;
  const int index = g_engine->zone_index(zone);
  if (index < 0) return false;
  switch (order) {
    case zonecontrol::Order::kOn: g_engine->set_zone_active(zone, true); break;
    case zonecontrol::Order::kOff: g_engine->set_zone_active(zone, false); break;
    case zonecontrol::Order::kToggle: g_engine->set_zone_active(zone, !g_engine->zone_is_active(static_cast<std::size_t>(index))); break;
    case zonecontrol::Order::kRelease: g_engine->release_latch(zone); break;
    case zonecontrol::Order::kNone: break;
  }
  g_states_due = true;
  return true;
}

const char* movement_of(int sensor_id, int track_id) {
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_engine || sensor_id < 1 || sensor_id > 255 || track_id < 1 || track_id > 255) return "still";
  return perimeter::movement_text(g_engine->movement_of(static_cast<std::uint8_t>(sensor_id), static_cast<std::uint8_t>(track_id)));
}

void set_armed(bool armed_now) { g_armed = armed_now; }
bool armed() { return g_armed; }

bool should_send_targets(const Track* tracks, std::size_t count, std::uint64_t now_ms) {
  std::lock_guard<std::mutex> guard(g_lock);
  return g_stream_gate.should_send(g_stream, tracks, count, now_ms);
}

bool learn_start(unsigned seconds) {
  if (seconds < 30) seconds = 30;
  if (seconds > 1800) seconds = 1800;
  std::lock_guard<std::mutex> guard(g_lock);
  if (!g_learner) g_learner = std::make_unique<reflectors::Learner>();
  g_learner->start(monotonic_ms(), seconds * 1000u);
  return true;
}

void learn_stop() {
  std::lock_guard<std::mutex> guard(g_lock);
  if (g_learner) g_learner->stop();
}

Summary summary() {
  Summary s;
  std::lock_guard<std::mutex> guard(g_lock);
  if (g_engine) {
    s.enabled = g_engine->config().enabled;
    s.masked = g_engine->masked();
    s.targets = g_engine->smoothed_count();
    s.raw_targets = g_engine->raw_count();
  }
  s.wifi_jam = g_jammed;
  s.buffered = g_buffered;
  s.sent = g_sent;
  s.replayed = g_replayed;
  s.limited = g_limited;
  s.dropped = g_dropped + (g_engine ? g_engine->dropped_events() : 0);
  s.flash_failures = g_flash_failures;
  return s;
}

std::string state_json() {
  json::Writer w;
  std::lock_guard<std::mutex> guard(g_lock);
  w.begin_object();
  if (!g_engine) return w.end_object().str();
  const perimeter::Config& c = g_engine->config();
  w.field("enabled", c.enabled).field("armed", g_armed.load()).field("masked", g_engine->masked()).field("wifi_jam", g_jammed).field("stream", perimeter::stream_text(c.stream))
      .field("targets", static_cast<long long>(g_engine->smoothed_count())).field("raw_targets", static_cast<long long>(g_engine->raw_count()));
  w.key("movement").begin_object().field("still", static_cast<long long>(g_engine->count_of(perimeter::Movement::kStill))).field("walking", static_cast<long long>(g_engine->count_of(perimeter::Movement::kWalking)))
      .field("running", static_cast<long long>(g_engine->count_of(perimeter::Movement::kRunning))).end_object();
  w.key("zones").begin_array();
  for (std::size_t z = 0; z < perimeter::kMaxZones; ++z) {
    const perimeter::Zone& zone = c.zones[z];
    if (!zone.enabled) continue;
    const perimeter::ZoneState& state = g_engine->zone(z);
    w.begin_object().field("name", zone.name).field("exclude", zone.exclude).field("active", g_engine->zone_is_active(z)).field("on", state.on).field("occupied", state.occupied).field("latched", state.latched)
        .field("occupants", static_cast<long long>(state.occupants)).field("entries", static_cast<long long>(state.entries)).field("last_dwell_s", static_cast<long long>(state.last_dwell_s)).end_object();
  }
  w.end_array().key("lines").begin_array();
  for (std::size_t l = 0; l < perimeter::kMaxLines; ++l) {
    if (!c.lines[l].enabled) continue;
    w.begin_object().field("name", c.lines[l].name).field("ins", static_cast<long long>(g_engine->line(l).ins)).field("outs", static_cast<long long>(g_engine->line(l).outs)).end_object();
  }
  w.end_array();
  w.key("events").begin_object().field("sent", static_cast<long long>(g_sent.load())).field("replayed", static_cast<long long>(g_replayed.load())).field("buffered", static_cast<long long>(g_buffered.load()))
      .field("limited", static_cast<long long>(g_limited.load())).field("dropped", static_cast<long long>(g_dropped.load() + g_engine->dropped_events())).field("flash_failures", static_cast<long long>(g_flash_failures.load())).end_object();
  w.key("learner").begin_object();
  const bool learning = g_learner && g_learner->active();
  w.field("active", learning).field("progress", static_cast<long long>(g_learner ? g_learner->progress_percent(monotonic_ms()) : 0)).field("frames", static_cast<long long>(g_learner ? g_learner->frames() : 0));
  w.key("suggestions").begin_array();
  if (g_learner && !learning) {
    reflectors::Suggestion found[reflectors::kMaxSuggestions];
    const std::size_t n = g_learner->suggest(found, reflectors::kMaxSuggestions);
    for (std::size_t i = 0; i < n; ++i) w.begin_object().field("x_mm", found[i].x_mm).field("y_mm", found[i].y_mm).field("width_mm", found[i].width_mm).field("height_mm", found[i].height_mm).field("frames", static_cast<long long>(found[i].frames)).end_object();
  }
  w.end_array().end_object();
  w.end_object();
  return w.str();
}

void metrics(std::vector<ZoneMetric>& zones, std::vector<LineMetric>& lines, std::size_t movement[3]) {
  std::lock_guard<std::mutex> guard(g_lock);
  movement[0] = movement[1] = movement[2] = 0;
  if (!g_engine) return;
  const perimeter::Config& c = g_engine->config();
  for (std::size_t z = 0; z < perimeter::kMaxZones; ++z) {
    if (!c.zones[z].enabled) continue;
    const perimeter::ZoneState& state = g_engine->zone(z);
    zones.push_back(ZoneMetric{c.zones[z].name, c.zones[z].exclude, state.on, state.occupied, state.occupants, state.entries});
  }
  for (std::size_t l = 0; l < perimeter::kMaxLines; ++l) if (c.lines[l].enabled) lines.push_back(LineMetric{c.lines[l].name, g_engine->line(l).ins, g_engine->line(l).outs});
  movement[0] = g_engine->count_of(perimeter::Movement::kStill);
  movement[1] = g_engine->count_of(perimeter::Movement::kWalking);
  movement[2] = g_engine->count_of(perimeter::Movement::kRunning);
}

}  // namespace armor::perimeter_rt
