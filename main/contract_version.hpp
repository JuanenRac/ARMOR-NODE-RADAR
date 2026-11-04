// The version of the contracts (ARMOR-COMMON) this firmware was last checked against by the reverse-dependency run of that repository. It goes out in the information message of the
// node so that a console can tell a node built against other contracts. ARMOR-COMMON holds the same number in registry/contract_pins.json and a test there fails when the two differ.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once

namespace armor {

inline constexpr const char* kContractVersion = "0.8.0.7";

}  // namespace armor
