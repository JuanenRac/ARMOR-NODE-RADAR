// ARMOR-NODE-RADAR - which request the web server is working on and since when, so that one that does not end is told. No hardware.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Each of the two servers (plain and HTTPS) answers one request at a time, so a request that does not end leaves the panel without an answer and nothing in the log. A timer looks at the
// slot of each server and says it once, as an error line: the health report takes it to the server even when the panel cannot be reached.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace armor::slowreq {

constexpr std::uint32_t kSlowMs = 5000;     // a request that ended after this long is said as a warning
constexpr std::uint32_t kStuckMs = 20000;   // one still running after this long is said, once, as an error

// What the log may say of a request: the method and the route, with no query and no name of a person.
inline std::string label(std::string_view method, std::string_view uri) {
  std::string_view path = uri.substr(0, uri.find('?'));
  constexpr std::string_view kPrefix = "/api/v1/";
  if (path.substr(0, kPrefix.size()) == kPrefix) path.remove_prefix(kPrefix.size());
  std::string route(path.substr(0, 32));
  if (route.compare(0, 6, "users/") == 0) route = "users/*";
  for (char& c : route) if (static_cast<unsigned char>(c) < 0x21 || static_cast<unsigned char>(c) > 0x7E) c = '?';
  return std::string(method) + " " + route;
}

// The firmware upload and the download of a core dump take as long as the link does: they are not told.
inline bool may_be_long(std::string_view what) { return what == "POST ota" || what == "GET coredump"; }

// One server's request. The caller holds its own lock.
struct Slot {
  std::string what;
  std::uint64_t since_ms = 0;
  bool busy = false;
  bool told = false;

  void begin(std::string request, std::uint64_t now_ms) {
    what = std::move(request);
    since_ms = now_ms;
    busy = true;
    told = false;
  }
  // How long the request took.
  std::uint64_t end(std::uint64_t now_ms) {
    if (!busy) return 0;
    busy = false;
    return now_ms >= since_ms ? now_ms - since_ms : 0;
  }
  // True once for a request that has been running for too long.
  bool stuck(std::uint64_t now_ms) {
    if (!busy || told || may_be_long(what) || now_ms < since_ms || now_ms - since_ms < kStuckMs) return false;
    told = true;
    return true;
  }
};

// The line of the log. It starts with what happened, so that it is still understood when the health report cuts it short.
inline std::string line(std::string_view what, std::uint64_t ms, bool still_running, std::uint32_t internal_free, std::uint32_t internal_largest) {
  return std::string(still_running ? "web stuck " : "web slow ") + std::to_string(ms / 1000) + " s: " + std::string(what) + " (internal " + std::to_string((internal_free + 512u) / 1024u) + "/" +
         std::to_string((internal_largest + 512u) / 1024u) + " KB)";
}

}  // namespace armor::slowreq
