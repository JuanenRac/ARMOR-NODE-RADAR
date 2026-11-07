// ARMOR-NODE-RADAR - tests/mqtt_shim.cpp: the real framing, decoding and message code of the node as a program for a computer, for the MQTT shim of ARMOR-SIMULATOR.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
//   mqtt_shim <node-id>      standard input, one chunk of the radar serial stream per line:  timestamp_ms <TAB> lux <TAB> hex-of-bytes
//                            standard output, one message per line:                          topic <TAB> payload
//
// It is built from the core files of the firmware (the same framer, decoder, track builder and JSON writers the node uses) and does not talk to any broker: the simulator
// publishes what it prints, so the server receives exactly the bytes the node would have sent. Nothing here has run on a board.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../main/core/frame_framer.hpp"
#include "../main/core/ld2450.hpp"
#include "../main/core/telemetry_json.hpp"

static std::vector<std::uint8_t> unhex(const std::string& hex) {
  std::vector<std::uint8_t> out;
  if (hex == "-") return out;
  for (std::size_t i = 0; i + 1 < hex.size(); i += 2) out.push_back(static_cast<std::uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
  return out;
}

static void emit(const std::string& node, const char* kind, const char* payload, std::size_t length) {
  char topic[96];
  if (!armor::build_topic(node, kind, topic, sizeof topic)) return;
  std::cout << topic << '\t' << std::string(payload, length) << '\n';
}

int main(int argc, char** argv) {
  const std::string node = argc > 1 ? argv[1] : "";
  char payload[1024];
  std::size_t length = 0;
  if (armor::build_info(node, 0, "Host shim", "0.0.0", "127.0.0.1", 80, payload, sizeof payload, length) == armor::JsonResult::kOk) emit(node, "info", payload, length);
  if (armor::build_health(node, 0, true, payload, sizeof payload, length) == armor::JsonResult::kOk) emit(node, "health", payload, length);
  armor::FrameFramer framer(armor::ld2450::protocol());
  std::string line;
  while (std::getline(std::cin, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream fields(line);
    std::string stamp, lux_text, hex;
    if (!std::getline(fields, stamp, '\t') || !std::getline(fields, lux_text, '\t') || !std::getline(fields, hex)) continue;
    const std::uint64_t timestamp = std::stoull(stamp);
    const float lux = std::stof(lux_text);
    const auto bytes = unhex(hex);
    framer.feed(bytes.data(), bytes.size(), [&](const std::uint8_t* data, std::size_t size) {
      armor::ld2450::Frame frame;
      if (!armor::ld2450::decode_frame(data, size, frame)) return;
      armor::Track tracks[armor::ld2450::kTargetsPerFrame];
      const std::size_t count = armor::ld2450::to_tracks(1, frame, tracks);
      char message[1024];
      std::size_t written = 0;
      if (armor::build_telemetry(node, timestamp, lux, tracks, count, message, sizeof message, written) == armor::JsonResult::kOk) emit(node, "telemetry", message, written);
    });
  }
  return 0;
}
