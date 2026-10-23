// ARMOR-NODE-RADAR - the temperature of the chip, from the sensor that is inside it.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once

namespace armor::chip {

// Degrees Celsius, or a value that is not a number (NAN) when the sensor could not be started or read. The sensor is started the first time this is called and stays on.
float temperature_c();

}  // namespace armor::chip
