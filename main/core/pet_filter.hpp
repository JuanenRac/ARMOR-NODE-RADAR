// ARMOR-RADAR - pet immunity for the presence sensors that report the energy of what they see. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// A cat or a small dog near the sensor makes a weak reflection: a short distance and a low energy, both for the moving and for the stationary target. The LD2410 and the LD2412 give both
// energies in every report (basic and engineering frames), so a reading that is near AND weak in both can be taken for an animal. A reading whose energy is unknown is never taken for
// one, and the filter is off unless the setting turns it on: it hides something real, so it must be a choice. The trackers (LD2450, LD2461) report no energy, so it does not apply to them.
#pragma once
#include "presence.hpp"

namespace armor::pet {

struct Rule {
  bool enabled = false;
  int max_distance_cm = 200;
  int max_energy = 30;
};

inline bool looks_like_pet(const presence::Reading& r, const Rule& rule) {
  if (!rule.enabled || !r.valid || !r.present || r.distance_cm < 0 || r.distance_cm > rule.max_distance_cm) return false;
  // every kind of target that exists must be weak (and an energy that is not known is not weak)
  const bool moving_weak = !r.moving || (r.moving_energy >= 0 && r.moving_energy <= rule.max_energy);
  const bool still_weak = !r.stationary || (r.static_energy >= 0 && r.static_energy <= rule.max_energy);
  return moving_weak && still_weak;
}

// The reading as the node will use it: the same, but nobody home when it looks like a pet.
inline presence::Reading filtered(const presence::Reading& r, const Rule& rule, bool& was_pet) {
  was_pet = looks_like_pet(r, rule);
  presence::Reading out = r;
  if (was_pet) { out.present = false; out.moving = false; out.stationary = false; out.state = 0; out.distance_cm = -1; }
  return out;
}

}  // namespace armor::pet
