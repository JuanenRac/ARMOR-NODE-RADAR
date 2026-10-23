// ARMOR-NODE-RADAR - what the node does when the store of its settings cannot be opened. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The settings store holds the node's settings and its users: erasing it sends the node back to set-up. A store that failed to open once may open after a restart (a power sag during a write is
// the usual cause), so the first failure restarts the node and only a failure that comes twice in a row, on two starts close together, erases the store. A start that opens it clears the count.
#pragma once
#include <cstdint>

namespace armor::storagerecovery {

enum class Action : std::uint8_t { kUse, kRestartAndRetry, kErase };

// `failures_before`: how many starts in a row already failed to open it (kept in memory that survives a software restart and is zero after a power-up).
inline Action decide(bool opened, unsigned failures_before) {
  if (opened) return Action::kUse;
  return failures_before == 0 ? Action::kRestartAndRetry : Action::kErase;
}

// The count after this start: zero when it opened, one more when it did not.
inline unsigned next_count(bool opened, unsigned failures_before) { return opened ? 0 : failures_before + 1; }

// The memory that survives a restart holds a magic number with the count, so that garbage there after a power-up is not taken for a count.
constexpr std::uint32_t kMagic = 0x53544F52;   // "STOR"
struct Memory { std::uint32_t magic; std::uint32_t failures; };
inline unsigned failures_in(const Memory& m) { return m.magic == kMagic && m.failures < 8 ? m.failures : 0; }
inline void store_failures(Memory& m, unsigned failures) { m.magic = kMagic; m.failures = failures; }

}  // namespace armor::storagerecovery
