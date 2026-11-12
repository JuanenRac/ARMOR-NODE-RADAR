// ARMOR-NODE-RADAR - host tests of the gateway watch: when a Wi-Fi link that looks connected is declared dead (rounds of pings to the gateway, a lost packet is not an outage).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// Nothing here touches hardware or a network: a "round" is the number of replies the test chooses.
#include <cstdint>
#include <cstdio>

#if __has_include("../core/link_watch.hpp")
#include "../core/link_watch.hpp"
#else
#include "../main/core/link_watch.hpp"
#endif

static int failures = 0;
static int checks = 0;
#define CHECK(condition)                                                              \
  do {                                                                                \
    ++checks;                                                                         \
    if (!(condition)) {                                                               \
      ++failures;                                                                     \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);                \
    }                                                                                 \
  } while (0)

using namespace armor::linkwatch;

// a round of 3 pings with `replies` answers
static Verdict ping_round(Watch& watch, std::uint32_t replies) { return watch.round(round_answered(replies, 3)); }

static void test_a_round_is_answered_with_one_reply() {
  CHECK(round_answered(3, 3) && round_answered(2, 3) && round_answered(1, 3));   // one of three is enough: a lost packet is not an outage
  CHECK(!round_answered(0, 3));
  CHECK(round_answered(0, 0));   // nothing was sent (no session, no memory): no verdict is made from a guess
  CHECK(round_answered(5, 0));
}

static void test_three_silent_rounds_in_a_row_reconnect() {
  Watch watch;
  CHECK(watch.limit() == 3 && watch.bad_rounds() == 0);
  CHECK(ping_round(watch, 0) == Verdict::kSuspect && watch.bad_rounds() == 1);
  CHECK(ping_round(watch, 0) == Verdict::kSuspect && watch.bad_rounds() == 2);
  CHECK(ping_round(watch, 0) == Verdict::kReconnect);
  CHECK(watch.bad_rounds() == 0);   // after the order, the fresh association starts from zero
  CHECK(ping_round(watch, 0) == Verdict::kSuspect && watch.bad_rounds() == 1);
}

static void test_one_answer_clears_the_count() {
  Watch watch;
  CHECK(ping_round(watch, 0) == Verdict::kSuspect);
  CHECK(ping_round(watch, 0) == Verdict::kSuspect);
  CHECK(ping_round(watch, 1) == Verdict::kHealthy && watch.bad_rounds() == 0);   // a single reply, two rounds in: not an outage
  CHECK(ping_round(watch, 0) == Verdict::kSuspect && watch.bad_rounds() == 1);   // the earlier silence is forgotten
  CHECK(ping_round(watch, 0) == Verdict::kSuspect);
  CHECK(ping_round(watch, 3) == Verdict::kHealthy);
}

static void test_losing_two_of_three_pings_forever_is_never_an_outage() {
  Watch watch;
  for (int i = 0; i < 1000; ++i) CHECK(ping_round(watch, 1) == Verdict::kHealthy);
}

static void test_idle_forgets_the_count() {
  Watch watch;
  CHECK(ping_round(watch, 0) == Verdict::kSuspect && ping_round(watch, 0) == Verdict::kSuspect);
  CHECK(watch.idle() == Verdict::kHealthy && watch.bad_rounds() == 0);   // the address was lost for a moment: the silence before it is not counted
  CHECK(ping_round(watch, 0) == Verdict::kSuspect && ping_round(watch, 0) == Verdict::kSuspect && ping_round(watch, 0) == Verdict::kReconnect);
}

static void test_the_limit_is_clamped_and_changeable() {
  Watch one(1);
  CHECK(one.round(false) == Verdict::kReconnect);   // 1: the first silent round already reconnects
  Watch zero(0);
  CHECK(zero.limit() == 1 && zero.round(false) == Verdict::kReconnect);
  Watch negative(-5);
  CHECK(negative.limit() == 1);
  Watch huge(100000);
  CHECK(huge.limit() == kMaxBadRounds);
  Watch five(5);
  for (int i = 0; i < 4; ++i) CHECK(five.round(false) == Verdict::kSuspect);
  CHECK(five.round(false) == Verdict::kReconnect);
  CHECK(five.round(true) == Verdict::kHealthy);   // the gateway does answer, so a later silence is an outage
  five.set_limit(2);
  CHECK(five.round(false) == Verdict::kSuspect && five.round(false) == Verdict::kReconnect);
}

static void test_lowering_the_limit_below_the_count_reconnects_at_the_next_silent_round() {
  Watch watch(10);
  for (int i = 0; i < 4; ++i) CHECK(watch.round(false) == Verdict::kSuspect);
  watch.set_limit(3);
  CHECK(watch.round(false) == Verdict::kReconnect && watch.bad_rounds() == 0);
}

static void test_defaults_are_the_ones_the_radar_node_has_always_used() {
  CHECK(kDefaultCheckEverySeconds == 20 && kDefaultBadRoundsBeforeReconnect == 3);
}

static void test_a_long_run_with_a_seeded_pattern() {
  // 5000 rounds from a fixed generator: the verdict must always follow the rule "reconnect exactly when 3 silent rounds came in a row".
  Watch watch;
  std::uint32_t x = 12345;
  int run = 0;
  for (int i = 0; i < 5000; ++i) {
    x = x * 1664525u + 1013904223u;
    const bool answered = ((x >> 24) % 5) != 0;   // about one round in five is silent
    const Verdict v = watch.round(answered);
    if (answered) { run = 0; CHECK(v == Verdict::kHealthy); }
    else if (++run == 3) { run = 0; CHECK(v == Verdict::kReconnect); }
    else CHECK(v == Verdict::kSuspect);
  }
}

// A router that never answers a ping (many drop ICMP): the node tries one fresh association, then stands down instead of reassociating every minute for ever.
static void test_a_router_that_never_answers_pings() {
  Watch watch;
  CHECK(!watch.gateway_answers_pings() && !watch.stood_down());
  for (int i = 0; i < 2; ++i) CHECK(ping_round(watch, 0) == Verdict::kSuspect);
  CHECK(ping_round(watch, 0) == Verdict::kReconnect && !watch.stood_down());   // the first time: a fresh association is worth a try
  for (int i = 0; i < 2; ++i) CHECK(ping_round(watch, 0) == Verdict::kSuspect);
  CHECK(ping_round(watch, 0) == Verdict::kSuspect && watch.stood_down());      // it did not help: no second order
  for (int i = 0; i < 300; ++i) CHECK(ping_round(watch, 0) == Verdict::kSuspect);   // and none later, however long the silence
  CHECK(ping_round(watch, 1) == Verdict::kHealthy && watch.gateway_answers_pings() && !watch.stood_down());   // an answer ends it
}

// A gateway that answered before and then goes silent is an outage: the watch keeps working for it, again and again.
static void test_a_gateway_that_answered_and_then_went_silent_is_always_an_outage() {
  Watch watch;
  CHECK(ping_round(watch, 3) == Verdict::kHealthy && watch.gateway_answers_pings());
  for (int cycle = 0; cycle < 10; ++cycle) {
    CHECK(ping_round(watch, 0) == Verdict::kSuspect);
    CHECK(ping_round(watch, 0) == Verdict::kSuspect);
    CHECK(ping_round(watch, 0) == Verdict::kReconnect && !watch.stood_down());
  }
}

// A new address (idle) starts the judgement again: a router that did not answer on the old network may answer on the new one.
static void test_a_new_address_forgets_that_the_watch_stood_down() {
  Watch watch;
  for (int i = 0; i < 12; ++i) ping_round(watch, 0);
  CHECK(watch.stood_down());
  watch.idle();
  CHECK(!watch.stood_down() && !watch.gateway_answers_pings() && watch.bad_rounds() == 0);
  for (int i = 0; i < 2; ++i) CHECK(ping_round(watch, 0) == Verdict::kSuspect);
  CHECK(ping_round(watch, 0) == Verdict::kReconnect);   // one fresh try again
}

// The way back to the first saved broker: never on the first broker, only when the time has come, and only when the first broker answers.
static void test_the_way_back_to_the_first_broker() {
  BackupReturn back(1800);
  int asked = 0;
  const auto answers = [&asked]() { ++asked; return true; };
  const auto down = [&asked]() { ++asked; return false; };
  for (int i = 0; i < 1000; ++i) CHECK(back.connected_round(0, 10, answers) == BackStep::kStay);   // on the first broker nothing is ever asked
  CHECK(asked == 0 && back.rounds() == 0);
  for (int i = 1; i < 180; ++i) CHECK(back.connected_round(1, 10, answers) == BackStep::kStay);   // 179 rounds of 10 s: 1790 s, not yet
  CHECK(asked == 0);
  CHECK(back.connected_round(1, 10, down) == BackStep::kFirstStillDown && asked == 1 && back.rounds() == 0);   // the 180th: asked, still down, the count starts again
  for (int i = 1; i < 180; ++i) CHECK(back.connected_round(2, 10, answers) == BackStep::kStay);
  CHECK(back.connected_round(2, 10, answers) == BackStep::kGoBack && asked == 2);                // answers: go back
  back.connected_round(1, 10, answers);
  back.reset();
  CHECK(back.rounds() == 0);
  BackupReturn never(0);                                                                         // a limit of zero is one second, never a division or a stall
  CHECK(never.connected_round(1, 0, answers) == BackStep::kGoBack);
  BackupReturn slow(100);
  CHECK(slow.connected_round(1, 60, answers) == BackStep::kStay && slow.connected_round(1, 60, answers) == BackStep::kGoBack);   // rounds of another length
  BackupReturn mixed(1800);
  for (int i = 0; i < 100; ++i) mixed.connected_round(1, 10, answers);
  CHECK(mixed.connected_round(0, 10, answers) == BackStep::kStay && mixed.rounds() == 0);        // back on the first broker by other means: the count is forgotten
}

int main() {
  test_the_way_back_to_the_first_broker();
  test_a_router_that_never_answers_pings();
  test_a_gateway_that_answered_and_then_went_silent_is_always_an_outage();
  test_a_new_address_forgets_that_the_watch_stood_down();
  test_a_round_is_answered_with_one_reply();
  test_three_silent_rounds_in_a_row_reconnect();
  test_one_answer_clears_the_count();
  test_losing_two_of_three_pings_forever_is_never_an_outage();
  test_idle_forgets_the_count();
  test_the_limit_is_clamped_and_changeable();
  test_lowering_the_limit_below_the_count_reconnects_at_the_next_silent_round();
  test_defaults_are_the_ones_the_radar_node_has_always_used();
  test_a_long_run_with_a_seeded_pattern();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
