// ARMOR-RADAR - who a sensor is: its firmware, its Bluetooth address and whether its own Bluetooth is open. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Sources: the command documents of the family (LD2450 manual 1.03 sections 2.2.6, 2.2.10 and 2.2.11; LD2410 protocol V1.00 sections 2.2.8, 2.2.12 and 2.2.13; LD2412 protocol V1.05
// sections 2.2.15, 2.2.19 and 2.2.20; LD2410S protocol V1.00 sections 2.2.2 and 2.2.6; MR24HPC1 manual V1.5 "Information inquiry"), and what other people who ran the sensors for
// a long time report about the answers (see docs/SENSORS.md). The worked examples of the documents are tests.
//
//   firmware (0x00A0)   status (2), type (2), major (2), minor (4)  -> "1.02.22062416"
//   Bluetooth address   (0x00A5, value 0x0001) status (2), six bytes. While the sensor's own Bluetooth is off it answers 08:05:04:03:02:01, so that address IS the state of the switch:
//                       the manual has no command that reads the switch itself.
//
// Some firmware versions answer a question with zeros or with all ones (the LD2412 V1.24 does not know its own version). That is "unknown", never a number, and never a reason to guess.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include "ld2450_command.hpp"
#include "sensor_model.hpp"

namespace armor::sensorid {

// ---- firmware ----------------------------------------------------------------------------------------------------------------------

struct Firmware {
  bool known = false;
  std::uint16_t type = 0;
  std::uint16_t major = 0;      // as the sensor sends it: 0x0102 is version 1.02
  std::uint32_t minor = 0;      // the build code, 0x22062416 = 2022-06-24 16h
  std::string text;             // "1.02.22062416", empty when not known
};

// The answer to 0x00A0 of the command family (LD2450, LD2410B/C, LD2412). A refusal, a short answer, all zeros and all ones are "not known".
inline Firmware firmware_from_ack(const ld2450cmd::Ack& ack) {
  Firmware f;
  if (ack.command != ld2450cmd::kReadFirmware || !ack.ok || ack.data_length < 8) return f;
  f.type = static_cast<std::uint16_t>(ack.data[0] | (ack.data[1] << 8));
  f.major = static_cast<std::uint16_t>(ack.data[2] | (ack.data[3] << 8));
  f.minor = static_cast<std::uint32_t>(ack.data[4]) | (static_cast<std::uint32_t>(ack.data[5]) << 8) | (static_cast<std::uint32_t>(ack.data[6]) << 16) | (static_cast<std::uint32_t>(ack.data[7]) << 24);
  if ((f.major == 0 && f.minor == 0) || (f.major == 0xFFFF && f.minor == 0xFFFFFFFFu) || f.major == 0xFFFF) return f;   // zeros and ones are what a sensor says when it does not know
  f.known = true;
  f.text = ld2450cmd::firmware_text(ack);
  return f;
}

// The LD2410S answers 0x0000 with three words (major, minor, patch) and no status word, so the "status" of the generic parser is its first word. The document does not say how the
// words are written, so they are shown as they come: "0100.0000.0000".
inline Firmware firmware_from_ld2410s_ack(const ld2450cmd::Ack& ack) {
  Firmware f;
  if (ack.command != 0x0000 || ack.data_length < 4) return f;
  const unsigned major = ack.status, minor = static_cast<unsigned>(ack.data[0] | (ack.data[1] << 8)), patch = static_cast<unsigned>(ack.data[2] | (ack.data[3] << 8));
  if ((major == 0 && minor == 0 && patch == 0) || (major == 0xFFFF && minor == 0xFFFF)) return f;
  char text[24];
  std::snprintf(text, sizeof text, "%04X.%04X.%04X", major, minor, patch);
  f.known = true;
  f.major = static_cast<std::uint16_t>(major);
  f.text = text;
  return f;
}

// The serial number of an LD2410S (0x0011): status, a length (2) and up to eight characters. Printable ASCII only.
inline std::string serial_from_ack(const ld2450cmd::Ack& ack) {
  if (ack.command != ld2450cmd::kReadSerialNumber || !ack.ok || ack.data_length < 2) return "";
  const std::size_t length = static_cast<std::size_t>(ack.data[0] | (ack.data[1] << 8));
  if (length == 0 || length > 16 || ack.data_length < 2 + length) return "";
  std::string out;
  for (std::size_t i = 0; i < length; ++i) {
    const std::uint8_t c = ack.data[2 + i];
    if (c < 0x20 || c > 0x7E) return "";
    out += static_cast<char>(c);
  }
  return out;
}

// ---- the Bluetooth address and the state of the switch --------------------------------------------------------------------------------

enum class Bluetooth : std::uint8_t { kUnknown, kOpen, kClosed, kNone };   // none: the model has no Bluetooth of its own

inline const char* bluetooth_text(Bluetooth state) {
  switch (state) {
    case Bluetooth::kOpen: return "open";
    case Bluetooth::kClosed: return "closed";
    case Bluetooth::kNone: return "none";
    case Bluetooth::kUnknown: break;
  }
  return "unknown";
}

struct Mac {
  bool known = false;
  std::uint8_t bytes[6]{};
  std::string text;          // "8F:27:2E:B8:0F:65"
  Bluetooth bluetooth = Bluetooth::kUnknown;
};

inline bool is_off_sentinel(const std::uint8_t* b) { return b[0] == 0x08 && b[1] == 0x05 && b[2] == 0x04 && b[3] == 0x03 && b[4] == 0x02 && b[5] == 0x01; }

// The answer to 0x00A5. The LD2410C document describes "a type byte and three bytes" and then gives a six byte example: six bytes are taken, and a seventh that is a leading zero type byte is skipped.
inline Mac mac_from_ack(const ld2450cmd::Ack& ack) {
  Mac mac;
  if (ack.command != ld2450cmd::kReadMac || !ack.ok) return mac;
  std::size_t at = 0;
  if (ack.data_length == 7 && ack.data[0] == 0x00) at = 1;
  else if (ack.data_length != 6) return mac;
  bool zeros = true, ones = true;
  for (std::size_t i = 0; i < 6; ++i) { mac.bytes[i] = ack.data[at + i]; zeros = zeros && mac.bytes[i] == 0x00; ones = ones && mac.bytes[i] == 0xFF; }
  if (zeros || ones) return mac;
  char text[20];
  std::snprintf(text, sizeof text, "%02X:%02X:%02X:%02X:%02X:%02X", mac.bytes[0], mac.bytes[1], mac.bytes[2], mac.bytes[3], mac.bytes[4], mac.bytes[5]);
  mac.text = text;
  mac.known = true;
  mac.bluetooth = is_off_sentinel(mac.bytes) ? Bluetooth::kClosed : Bluetooth::kOpen;
  if (mac.bluetooth == Bluetooth::kClosed) mac.text.clear();   // the sentinel is not an address
  return mac;
}

// Whether a model has a Bluetooth of its own that this node can switch (the three of the command family that carry one).
inline bool has_own_bluetooth(sensors::Model model) { return model == sensors::Model::kLd2450 || model == sensors::Model::kLd2410 || model == sensors::Model::kLd2412; }

// ---- what a model's firmware is known to do ----------------------------------------------------------------------------------------

// The LD2412 versions 1.24 and 1.26 (the common ones) acknowledge commands they do not carry out: the serial speed, the engineering mode, and the distance resolution, which on them
// breaks the link. Version 1.24 does not even answer the version query. Only 1.36 is known to work, and it is not distributed. An unknown version is treated like the bad ones.
enum class Ld2412Level : std::uint8_t { kNotLd2412, kBad, kUnknown, kGood };

inline Ld2412Level ld2412_level(sensors::Model model, const Firmware& firmware) {
  if (model != sensors::Model::kLd2412) return Ld2412Level::kNotLd2412;
  if (!firmware.known) return Ld2412Level::kUnknown;
  if (firmware.major == 0x0124 || firmware.major == 0x0126) return Ld2412Level::kBad;
  if (firmware.major >= 0x0136) return Ld2412Level::kGood;
  return Ld2412Level::kUnknown;
}

struct Ld2412Policy {
  bool may_set_baud = true;
  bool may_set_resolution = true;
  bool trust_engineering_ack = true;
  bool verify_gates = false;          // read the distance gates back ~40 s after the sensor started and set them again once if they changed
  std::uint32_t verify_after_ms = 40000;
  bool warn = false;                  // the panel and Studio say which firmware the sensor has and what it cannot do
};

inline Ld2412Policy ld2412_policy(sensors::Model model, const Firmware& firmware) {
  Ld2412Policy policy;
  const Ld2412Level level = ld2412_level(model, firmware);
  if (level == Ld2412Level::kNotLd2412 || level == Ld2412Level::kGood) return policy;
  policy.may_set_baud = false;
  policy.may_set_resolution = false;
  policy.trust_engineering_ack = false;
  policy.verify_gates = true;
  policy.warn = true;
  return policy;
}

// A node never changes the serial speed of any sensor (a wrong choice leaves it unreachable), sets the resolution of an LD2412 only when its version is known to be good, and does not trust the
// answer of an LD2412 that is not. This is the one place that says so; the node has no command for the first two today, and run() asks here as a second wall for any that is added.
enum class Risky : std::uint8_t { kSetBaud, kSetResolution, kEngineeringMode };
inline bool command_allowed(sensors::Model model, const Firmware& firmware, Risky what) {
  const Ld2412Policy policy = ld2412_policy(model, firmware);
  switch (what) {
    case Risky::kSetBaud: return false;
    case Risky::kSetResolution: return policy.may_set_resolution;
    case Risky::kEngineeringMode: return policy.trust_engineering_ack;
  }
  return false;
}

// What the node tells about one sensor.
struct Identity {
  Firmware firmware;
  std::string serial;                    // LD2410S
  Mac mac;
  Bluetooth bluetooth = Bluetooth::kUnknown;
  std::uint64_t read_at_ms = 0;          // when the node last read it; 0: never
};

// The Bluetooth state a model is shown with before anything is read.
inline Bluetooth initial_bluetooth(sensors::Model model) { return has_own_bluetooth(model) ? Bluetooth::kUnknown : Bluetooth::kNone; }

}  // namespace armor::sensorid
