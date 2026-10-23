// ARMOR-NODE-RADAR - the node's numbers for Prometheus (GET /metrics, administrators only). The format is in core/metrics.hpp (tested on a computer).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <string>

namespace armor::metrics_page {

// The whole page, read from the node as it is now.
std::string text();

}  // namespace armor::metrics_page
