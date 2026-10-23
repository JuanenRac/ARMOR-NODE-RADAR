// ARMOR-NODE-RADAR - tells when the Wi-Fi signal collapses the way a jammer makes it. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The signal strength of the access point the node uses is followed with a slow average (the baseline, which only moves on samples that look normal). A sample far under the baseline, or no
// link at all, is suspicious; two suspicious samples in a row confirm it (one is a passing car), and three normal ones in a row end it. A router that restarts looks the same for a minute: this tells
// what the radio sees, and the server decides what to do with it.
#pragma once
#include <cstdint>

namespace armor::jam {

struct Rule {
  int drop_db = 15;          // how far under the baseline counts as a collapse
  int confirm = 2;           // suspicious samples in a row
  int clear_after = 3;       // normal samples in a row
  int alpha_percent = 5;     // weight of a normal sample in the baseline
};

class Detector {
 public:
  explicit Detector(const Rule& rule = Rule{}) : rule_(rule) {}

  // One sample: the strength in dBm (negative; 0 or positive means "no reading") and whether the link is up. Returns +1 when a jam has just been confirmed, -1 when it has just ended, else 0.
  int feed(int rssi_dbm, bool link_up) {
    const bool reading = rssi_dbm < 0 && rssi_dbm >= -120;
    if (link_up && !reading) return 0;   // an up link without a reading says nothing
    const bool suspicious = !link_up || (known_ && rssi_dbm < baseline_ - rule_.drop_db);
    if (!suspicious) {
      if (!known_) { baseline_ = rssi_dbm; known_ = true; }
      else if (!jammed_) baseline_ += (rssi_dbm - baseline_) * rule_.alpha_percent / 100.0;
      bad_ = 0;
      if (jammed_ && ++good_ >= rule_.clear_after) { jammed_ = false; good_ = 0; return -1; }
      return 0;
    }
    good_ = 0;
    if (!known_) return 0;   // nothing to compare with yet
    if (++bad_ >= rule_.confirm && !jammed_) { jammed_ = true; return 1; }
    return 0;
  }

  bool jammed() const { return jammed_; }
  bool known() const { return known_; }
  double baseline_dbm() const { return baseline_; }

 private:
  Rule rule_;
  double baseline_ = 0;
  bool known_ = false;
  bool jammed_ = false;
  int bad_ = 0, good_ = 0;
};

}  // namespace armor::jam
