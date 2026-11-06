// ARMOR-NODE-RADAR - tests/sim_bench.cpp: the driver of the simulation bench. The serial stream of the HLK-LD2450 modules goes through the real framer and decoder.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// It is built from the core files of the firmware, on a computer, and fed by ARMOR-SIMULATOR (python -m armor_simulator.node_bench) with the buses of the equipment, spoiled
// on purpose. Nothing here has run on a board.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include "../main/core/frame_framer.hpp"
#include "../main/core/ld2450.hpp"

// The line protocol shared by the drivers of every node (see ARMOR-SIMULATOR docs/NODE_BENCH.md): one case per line on the standard input,
//   bus <TAB> id <TAB> arg <TAB> payload        (payload: hex of bytes or text; "-" when there is none)
// and one answer per case on the standard output: id <TAB> answer.
namespace simio {
inline std::vector<std::string> split(const std::string& text, char sep) {
  std::vector<std::string> out;
  std::string item;
  for (char c : text) { if (c == sep) { out.push_back(item); item.clear(); } else item += c; }
  out.push_back(item);
  return out;
}
inline std::vector<std::uint8_t> unhex(const std::string& hex) {
  std::vector<std::uint8_t> out;
  if (hex == "-") return out;
  for (std::size_t i = 0; i + 1 < hex.size(); i += 2) out.push_back(static_cast<std::uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
  return out;
}
inline std::string unhex_text(const std::string& hex) { const auto bytes = unhex(hex); return std::string(bytes.begin(), bytes.end()); }
inline std::string fixed(double value, int digits) { char buffer[64]; std::snprintf(buffer, sizeof buffer, "%.*f", digits, value); return buffer; }
struct Case { std::string bus, id, arg, payload; };
inline bool next_case(Case& out) {
  std::string line;
  while (std::getline(std::cin, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const auto f = split(line, '\t');
    if (f.size() != 4) continue;
    out = Case{f[0], f[1], f[2], f[3]};
    return true;
  }
  return false;
}
}  // namespace simio

static const char* const BUS = "ld2450";

static const char* refusal(armor::ld2450::Reject why) {
  switch (why) {
    case armor::ld2450::Reject::kNoResolution: return "no-resolution";
    case armor::ld2450::Reject::kOutOfRange: return "out-of-range";
    case armor::ld2450::Reject::kOrigin: return "origin";
    default: return "";
  }
}

// The whole stream, handed over in the pieces the payload says (comma separated), through one framer, as the serial driver does.
static std::string answer(const simio::Case& c) {
  armor::FrameFramer framer(armor::ld2450::protocol());
  std::vector<std::string> frames;
  const auto on_frame = [&](const std::uint8_t* data, std::size_t length) {
    armor::ld2450::Frame frame;
    if (!armor::ld2450::decode_frame(data, length, frame)) return;
    armor::Track tracks[armor::ld2450::kTargetsPerFrame];
    const std::size_t count = armor::ld2450::to_tracks(1, frame, tracks);
    std::string text = "tracks=";
    for (std::size_t i = 0; i < count; ++i) text += std::string(i ? "," : "") + std::to_string(tracks[i].track_id) + ":" + std::to_string(tracks[i].x_mm) + ":" + std::to_string(tracks[i].y_mm) + ":" + std::to_string(tracks[i].speed_mm_s);
    if (count == 0) text += "-";
    std::string refused;
    for (std::size_t slot = 0; slot < armor::ld2450::kTargetsPerFrame; ++slot) {
      if (frame.targets[slot].present && !frame.targets[slot].valid) refused += std::string(refused.empty() ? "" : ",") + std::to_string(slot + 1) + ":" + refusal(frame.targets[slot].rejected);
    }
    frames.push_back(text + " refused=" + (refused.empty() ? "-" : refused));
  };
  for (const std::string& piece : simio::split(c.payload, ',')) {
    const auto bytes = simio::unhex(piece);
    framer.feed(bytes.data(), bytes.size(), on_frame);
  }
  std::string text = "frames=" + std::to_string(frames.size());
  for (std::size_t i = 0; i < frames.size(); ++i) text += (i ? "|" : " ") + frames[i];
  return text;
}

int main() {
  simio::Case c;
  while (simio::next_case(c)) std::cout << c.id << '\t' << (c.bus == BUS ? answer(c) : std::string("unsupported")) << '\n';
  return 0;
}
