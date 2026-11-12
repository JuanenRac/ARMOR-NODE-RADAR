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
constexpr int kMaxFruitlessReconnects = 1;           // a gateway that has NEVER answered a ping since the node got its address may simply not answer ICMP: one fresh association is tried, then the watch stands down

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
  // True once the gateway has answered at least one round since the node got its address (so a later silence is an outage and not a router that does not answer pings).
  bool gateway_answers_pings() const { return answered_; }
  // True when the watch has stood down: the gateway never answered and a fresh association did not change that. It is not an outage the node can cure; a reply (or a new address) ends it.
  bool stood_down() const { return fruitless_ > kMaxFruitlessReconnects; }

  // The node has no address, no gateway or is not a station: nothing to judge, forget the count.
  Verdict idle() { bad_ = 0; answered_ = false; fruitless_ = 0; return Verdict::kHealthy; }

  // The outcome of one round to the gateway.
  Verdict round(bool answered) {
    if (answered) { bad_ = 0; answered_ = true; fruitless_ = 0; return Verdict::kHealthy; }
    bad_ += 1;
    if (bad_ >= limit_) {
      bad_ = 0;   // after the order, count from zero: the fresh association gets its own chance
      if (!answered_ && ++fruitless_ > kMaxFruitlessReconnects) return Verdict::kSuspect;   // a router that never answers a ping is not "dead": do not reassociate every minute for ever
      return Verdict::kReconnect;
    }
    return Verdict::kSuspect;
  }

 private:
  int limit_ = kDefaultBadRoundsBeforeReconnect;
  int bad_ = 0;
  bool answered_ = false;
  int fruitless_ = 0;
};

// The way back to the first saved broker. A node that works on a backup broker asks, once in a while, whether the first one answers again: it only goes back when it does, because leaving a broker
// that works for one that is still down costs a minute of silence every time. The rule is here so that it is tested on a computer; the node's task counts its rounds and does the connecting.
enum class BackStep {
  kStay,           // nothing to decide this round
  kFirstStillDown, // the time has come, the first broker did not answer: stay where it works
  kGoBack,         // the time has come and the first broker answers: go back to it
};

class BackupReturn {
 public:
  explicit BackupReturn(int after_seconds = 1800) : after_seconds_(after_seconds < 1 ? 1 : after_seconds) {}
  // One round of a connected node (`every_seconds` apart). `broker_index` 0 is the first saved broker. `first_answers` is asked only when the time has come.
  template <class Ask>
  BackStep connected_round(int broker_index, int every_seconds, Ask first_answers) {
    if (broker_index == 0) { rounds_ = 0; return BackStep::kStay; }
    if (++rounds_ * (every_seconds < 1 ? 1 : every_seconds) < after_seconds_) return BackStep::kStay;
    rounds_ = 0;
    return first_answers() ? BackStep::kGoBack : BackStep::kFirstStillDown;
  }
  // The connection went down (or the broker changed): the count starts again.
  void reset() { rounds_ = 0; }
  int rounds() const { return rounds_; }

 private:
  int after_seconds_;
  int rounds_ = 0;
};

}  // namespace armor::linkwatch
