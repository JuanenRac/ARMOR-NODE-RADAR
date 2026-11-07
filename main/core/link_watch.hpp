// ARMOR-NODE-RADAR - the rule of the gateway watch: when the Wi-Fi station says it is connected but the gateway stops answering, the link is dead and only a fresh association brings it back.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The radio can stay "connected" (no disconnect event, a good signal) while the node is no longer reachable: the gateway itself stops answering it until the node is reset by hand. The
// network code pings the gateway from time to time and gives this class the outcome of each round; the class decides. One lost packet is not an outage: a round counts as answered when AT
// LEAST ONE of its pings got a reply, and the link is declared dead only after several rounds in a row with none. This file touches no hardware, so the rule is tested on a computer.
#pragma once
#include <cstdint>

namespace armor::linkwatch {

constexpr int kDefaultCheckEverySeconds = 20;        // one round every 20 s
constexpr int kDefaultBadRoundsBeforeReconnect = 3;  // three rounds in a row without a single reply: about a minute of silence
constexpr int kMaxBadRounds = 20;                    // anything above is clamped: a watch that never fires is the same as no watch

// What a round did to the watch.
enum class Verdict {
  kHealthy,    // the gateway answered (or the watch has nothing to judge): the count of bad rounds is zero
  kSuspect,    // a round without any reply, but not enough of them yet
  kReconnect,  // enough rounds in a row without any reply: force a fresh association now
};

// True when a round of pings counts as answered: at least one reply. `sent` is how many pings the round sent; a round that could not even send (0) is not judged as bad.
inline bool round_answered(std::uint32_t replies, std::uint32_t sent) { return sent == 0 || replies > 0; }

class Watch {
 public:
  explicit Watch(int bad_rounds_before_reconnect = kDefaultBadRoundsBeforeReconnect) { set_limit(bad_rounds_before_reconnect); }

  void set_limit(int bad_rounds) { limit_ = bad_rounds < 1 ? 1 : (bad_rounds > kMaxBadRounds ? kMaxBadRounds : bad_rounds); }
  int limit() const { return limit_; }
  int bad_rounds() const { return bad_; }

  // The node has no address, no gateway or is not a station: nothing to judge, forget the count.
  Verdict idle() { bad_ = 0; return Verdict::kHealthy; }

  // The outcome of one round to the gateway.
  Verdict round(bool answered) {
    if (answered) { bad_ = 0; return Verdict::kHealthy; }
    bad_ += 1;
    if (bad_ >= limit_) { bad_ = 0; return Verdict::kReconnect; }   // after the order, count from zero: the fresh association gets its own chance
    return Verdict::kSuspect;
  }

 private:
  int limit_ = kDefaultBadRoundsBeforeReconnect;
  int bad_ = 0;
};

}  // namespace armor::linkwatch
