// ARMOR-NODE-RADAR - host tests for the small tables that the other tests never reached: every name of an event and of a refusal, every layout of the network, every
// setting of the light sensor and the text a presence sensor gives the server.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <set>
#include <string>
#include "../main/core/network_plan.hpp"
#include "../main/core/perimeter.hpp"
#include "../main/core/presence.hpp"
#include "../main/core/radar_event.hpp"
#include "../main/core/sensor_identity.hpp"
#include "../main/core/serial_bridge.hpp"
#include "../main/core/veml7700.hpp"

static int failures = 0;
static int checks = 0;
#define CHECK(condition)                                                              \
  do {                                                                                \
    ++checks;                                                                         \
    if (!(condition)) {                                                               \
      ++failures;                                                                     \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);                \
    }                                                                                 \
  } while (0)

using namespace armor;

static void test_event_names() {
  using radarevent::Kind;
  const Kind all[] = {Kind::kZoneOccupied, Kind::kZoneClear, Kind::kLoiter, Kind::kTripwire, Kind::kPathViolation, Kind::kMasking, Kind::kMaskingEnd,
                      Kind::kSensorBluetoothOpen, Kind::kSensorBluetoothClosed, Kind::kWifiJam, Kind::kWifiJamEnd, Kind::kBridgeOpened, Kind::kBridgeClosed};
  std::set<std::string> names;
  for (Kind kind : all) {
    const std::string name = radarevent::kind_text(kind);
    CHECK(!name.empty());
    names.insert(name);
  }
  CHECK(names.size() == sizeof(all) / sizeof(all[0]));   // no two events share a word
  CHECK(names.count("wifi_jam_end") == 1 && names.count("bridge_closed") == 1 && names.count("sensor_bluetooth_closed") == 1);

  using Type = perimeter::Event::Type;
  CHECK(radarevent::kind_of(Type::kZoneOccupied) == Kind::kZoneOccupied);
  CHECK(radarevent::kind_of(Type::kZoneClear) == Kind::kZoneClear);
  CHECK(radarevent::kind_of(Type::kLoiter) == Kind::kLoiter);
  CHECK(radarevent::kind_of(Type::kTripwire) == Kind::kTripwire);
  CHECK(radarevent::kind_of(Type::kPathViolation) == Kind::kPathViolation);
  CHECK(radarevent::kind_of(Type::kMasking) == Kind::kMasking);
  CHECK(radarevent::kind_of(Type::kMaskingEnd) == Kind::kMaskingEnd);
  CHECK(radarevent::topic("not a valid id!").empty());
}

static void test_layout_names() {
  using netplan::Layout;
  const Layout all[] = {Layout::kEthernet, Layout::kEthernetBridgedAp, Layout::kEthernetWithAp, Layout::kWifiStation, Layout::kWifiStationWithAp};
  std::set<std::string> names;
  for (Layout layout : all) names.insert(netplan::to_text(layout));
  CHECK(names.size() == 5);
  CHECK(std::string(netplan::to_text(Layout::kEthernet)) == "ethernet");
  CHECK(std::string(netplan::to_text(Layout::kEthernetBridgedAp)) == "ethernet+ap-bridged");
  CHECK(std::string(netplan::to_text(Layout::kEthernetWithAp)) == "ethernet+ap");
  CHECK(std::string(netplan::to_text(Layout::kWifiStation)) == "wifi-station");
  CHECK(std::string(netplan::to_text(Layout::kWifiStationWithAp)) == "wifi-station+ap");
}

static void test_bridge_refusals() {
  using bridge::Refusal;
  const Refusal all[] = {Refusal::kBusy, Refusal::kBadSensor, Refusal::kBadBaud, Refusal::kBadMinutes, Refusal::kNoSession, Refusal::kBadToken, Refusal::kExpired, Refusal::kTooBig, Refusal::kTooFast};
  std::set<std::string> names;
  for (Refusal refusal : all) {
    const std::string text = bridge::refusal_text(refusal);
    CHECK(text.rfind("bridge_", 0) == 0);   // every refusal is a word the panel has a text for
    names.insert(text);
  }
  CHECK(names.size() == 9);
  CHECK(std::string(bridge::refusal_text(Refusal::kNone)).empty());
}

static void test_light_sensor_settings() {
  using namespace veml7700;
  // each integration time has its own register bits and its own length, and a longer time resolves finer
  const Integration times[] = {Integration::kMs25, Integration::kMs50, Integration::kMs100, Integration::kMs200, Integration::kMs400, Integration::kMs800};
  const std::uint32_t lengths[] = {25, 50, 100, 200, 400, 800};
  std::set<unsigned> bits;
  for (std::size_t i = 0; i < 6; ++i) {
    CHECK(integration_ms(times[i]) == lengths[i]);
    bits.insert(integration_bits(times[i]));
  }
  CHECK(bits.size() == 6);
  CHECK(integration_bits(Integration::kMs50) == 0b1000 && integration_bits(Integration::kMs400) == 0b0010 && integration_bits(Integration::kMs800) == 0b0011);
  CHECK(gain_factor(Gain::kX1) == 1.0f && gain_factor(Gain::kX2) == 2.0f && gain_factor(Gain::kX1_8) == 0.125f && gain_factor(Gain::kX1_4) == 0.25f);
  CHECK(resolution_lux_per_count({Gain::kX2, Integration::kMs800}) < resolution_lux_per_count({Gain::kX1, Integration::kMs100}));
  // the register word keeps the gain in bits 12..11 and the time in bits 9..6
  CHECK(config_word({Gain::kX2, Integration::kMs800}) == ((1u << 11) | (0b0011u << 6)));
  CHECK(settle_ms({Gain::kX1, Integration::kMs100}) == 240);
  // at the ends of the ladder the step does not run off it
  CHECK(next_step(0, 60001) == 0);
  CHECK(next_step(kLadderSteps - 1, 5) == kLadderSteps - 1);
  CHECK(reading_is_trustworthy(kStartStep, 1000));
  CHECK(!reading_is_trustworthy(kStartStep, 5));
}

static void test_presence_payload_and_bluetooth_words() {
  CHECK(presence::device_payload(true, 240) == "{\"triggered\":true,\"distance_cm\":240}");
  CHECK(presence::device_payload(false, -1) == "{\"triggered\":false}");
  CHECK(presence::device_payload(true, 0) == "{\"triggered\":true,\"distance_cm\":0}");
  CHECK(std::string(sensorid::bluetooth_text(sensorid::Bluetooth::kClosed)) == "closed");
  CHECK(std::string(sensorid::bluetooth_text(sensorid::Bluetooth::kNone)) == "none");
}

int main() {
  test_event_names();
  test_layout_names();
  test_bridge_refusals();
  test_light_sensor_settings();
  test_presence_payload_and_bluetooth_words();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
