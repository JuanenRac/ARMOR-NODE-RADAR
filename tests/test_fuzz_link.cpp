// ARMOR-NODE-RADAR - host tests with a fixed seed: the readers of what arrives from outside (Bluetooth frames, JSON, Improv packets, broker addresses) must never crash nor accept noise,
// and the rules of a lost link (the gateway watch, the retry wait, the way back to the first broker) must behave in any run of lost and answered rounds.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#if __has_include("../core/ble_frame.hpp")
#include "../core/ble_frame.hpp"
#include "../core/broker_uri.hpp"
#include "../core/backoff.hpp"
#include "../core/improv.hpp"
#include "../core/json.hpp"
#include "../core/link_watch.hpp"
#else
#include "../main/core/ble_frame.hpp"
#include "../main/core/broker_uri.hpp"
#include "../main/core/backoff.hpp"
#include "../main/core/improv.hpp"
#include "../main/core/json.hpp"
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

// the same numbers on every run, so a failure can be repeated
static std::uint32_t rng_state = 0x2545F491u;
static std::uint32_t next_random() {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return rng_state;
}
static std::uint32_t below(std::uint32_t limit) { return limit == 0 ? 0 : next_random() % limit; }

static std::string noise(std::size_t length) {
  std::string out;
  for (std::size_t i = 0; i < length; ++i) out.push_back(static_cast<char>(next_random() & 0xff));
  return out;
}

// a valid text with a few bytes changed, dropped or added
static std::string mutate(std::string text) {
  const std::uint32_t edits = 1 + below(4);
  for (std::uint32_t i = 0; i < edits && !text.empty(); ++i) {
    const std::size_t at = below(static_cast<std::uint32_t>(text.size()));
    switch (below(3)) {
      case 0: text[at] = static_cast<char>(next_random() & 0xff); break;
      case 1: text.erase(at, 1); break;
      default: text.insert(at, 1, static_cast<char>(next_random() & 0xff)); break;
    }
  }
  return text;
}

using namespace armor;

static void bluetooth_frames() {
  const std::string request = "{\"id\":7,\"op\":\"config.get\"}";
  const std::string framed = ble::frame(request);
  CHECK(framed.size() == request.size() + ble::kHeader);
  CHECK(ble::frame("").empty() && ble::frame(std::string(ble::kMaxMessage + 1, 'x')).empty());
  CHECK(!ble::frame(std::string(ble::kMaxMessage, 'x')).empty());
  CHECK(ble::chunks_of(framed, 0).empty());

  // a message cut in pieces of any size comes out whole and only at the last byte
  for (int round = 0; round < 300; ++round) {
    ble::Assembler assembler;
    const std::size_t chunk = 1 + below(40);
    std::string got;
    bool complete = false;
    const std::vector<std::string> pieces = ble::chunks_of(framed, chunk);
    for (std::size_t i = 0; i < pieces.size(); ++i) {
      std::string message;
      const bool done = assembler.feed(reinterpret_cast<const std::uint8_t*>(pieces[i].data()), pieces[i].size(), message);
      if (done) {
        complete = true;
        got = message;
        CHECK(i + 1 == pieces.size());
      }
    }
    CHECK(complete && got == request && assembler.pending() == 0 && assembler.errors() == 0);
  }

  // two messages written back to back, in pieces of any size
  const std::string second = "{\"id\":8,\"op\":\"login\",\"args\":{\"user\":\"x\"}}";
  for (int round = 0; round < 100; ++round) {
    const std::string both = framed + ble::frame(second);
    ble::Assembler assembler;
    std::vector<std::string> messages;
    const std::size_t chunk = 1 + below(30);
    for (const std::string& piece : ble::chunks_of(both, chunk)) {
      std::string message;
      bool done = assembler.feed(reinterpret_cast<const std::uint8_t*>(piece.data()), piece.size(), message);
      while (done) {
        messages.push_back(message);
        done = assembler.take(message);
      }
    }
    CHECK(messages.size() == 2 && messages[0] == request && messages[1] == second && assembler.pending() == 0);
  }

  // a length of zero or above the largest is an error: the assembler drops everything, and a message sent after reset() is read
  ble::Assembler broken;
  std::string message;
  const std::uint8_t zero[3] = {0, 0, 'x'};
  CHECK(!broken.feed(zero, 3, message) && broken.errors() == 1 && broken.pending() == 0);
  const std::uint8_t huge[2] = {0xFF, 0xFF};
  CHECK(!broken.feed(huge, 2, message) && broken.errors() == 2 && broken.pending() == 0);
  broken.reset();
  CHECK(broken.feed(reinterpret_cast<const std::uint8_t*>(framed.data()), framed.size(), message) && message == request);

  // noise: never more kept than one message and its header, never a crash
  for (int round = 0; round < 400; ++round) {
    ble::Assembler assembler;
    for (int step = 0; step < 12; ++step) {
      const std::string bytes = noise(1 + below(60));
      std::string out;
      bool done = assembler.feed(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size(), out);
      while (done) {
        CHECK(!out.empty() && out.size() <= ble::kMaxMessage);
        done = assembler.take(out);
      }
      CHECK(assembler.pending() <= ble::kHeader + ble::kMaxMessage + 60);
    }
  }

  // requests: a mutated request is read or refused, a cut one is never read, and what is read has a number for an id
  ble::Request good;
  CHECK(ble::parse_request(request, good) && good.id == 7 && good.op == "config.get" && good.args.is_object());
  for (int round = 0; round < 1500; ++round) {
    ble::Request parsed;
    if (ble::parse_request(mutate(request), parsed)) CHECK(parsed.id >= 0 && parsed.args.is_object());
  }
  for (std::size_t cut = 0; cut < request.size(); ++cut) {
    ble::Request parsed;
    CHECK(!ble::parse_request(request.substr(0, cut), parsed));
  }
  for (int round = 0; round < 500; ++round) {
    ble::Request parsed;
    CHECK(!ble::parse_request(noise(1 + below(40)), parsed));
  }
  ble::Request odd;
  CHECK(!ble::parse_request("{\"id\":-1,\"op\":\"x\"}", odd) && !ble::parse_request("{\"id\":1.5,\"op\":\"x\"}", odd) && !ble::parse_request("{\"id\":\"1\",\"op\":\"x\"}", odd));
  CHECK(!ble::parse_request("{\"id\":1}", odd) && !ble::parse_request("{\"op\":\"x\"}", odd) && !ble::parse_request("[1,2]", odd) && !ble::parse_request("{\"id\":1,\"op\":5}", odd));
  CHECK(!ble::parse_request("{\"id\":1,\"op\":\"\"}", odd) && !ble::parse_request("{\"id\":1,\"op\":\"" + std::string(33, 'a') + "\"}", odd));
  CHECK(!ble::parse_request("{\"id\":1e300,\"op\":\"x\"}", odd) && !ble::parse_request("{\"id\":9007199254740993,\"op\":\"x\"}", odd));
}

static void json_noise() {
  const std::string document = "{\"a\":[1,2,{\"b\":null,\"c\":\"text\"}],\"d\":true,\"e\":-1.5e2,\"f\":{\"g\":[]}}";
  json::Value value;
  CHECK(json::parse(document, value) && value.is_object());
  for (std::size_t cut = 0; cut < document.size(); ++cut) CHECK(!json::parse(document.substr(0, cut), value));   // a cut text is never a document
  CHECK(!json::parse(document + " x", value) && !json::parse(document + document, value));
  CHECK(!json::parse("{\"a\":1,}", value) && !json::parse("[1,]", value) && !json::parse("{'a':1}", value) && !json::parse("// c\n{}", value));
  for (int round = 0; round < 3000; ++round) {
    json::Value parsed;
    if (json::parse(mutate(document), parsed)) CHECK(parsed.is_object() || parsed.is_array() || parsed.is_string() || parsed.is_number() || parsed.is_bool() || parsed.type == json::Type::kNull);
  }
  for (int round = 0; round < 1500; ++round) {
    json::Value parsed;
    json::parse(noise(below(200)), parsed);
  }
  // the depth, the size and the number of values are bounded
  CHECK(json::parse(std::string(3, '[') + std::string(3, ']'), value));
  CHECK(!json::parse(std::string(json::kMaxDepth + 5, '[') + std::string(json::kMaxDepth + 5, ']'), value));
  CHECK(!json::parse(std::string(5000, '['), value));
  std::string many = "[";
  for (std::size_t i = 0; i < json::kMaxValues + 10; ++i) many += i == 0 ? "0" : ",0";
  many += "]";
  CHECK(!json::parse(many, value));
  CHECK(!json::parse("\"" + std::string(json::kMaxText + 1, 'a') + "\"", value));
  // numbers outside what a setting accepts fall back to the default
  json::Value numbers;
  CHECK(json::parse("{\"n\":5,\"big\":1e30,\"frac\":2.5,\"text\":\"5\"}", numbers));
  CHECK(numbers.integer_or("n", -1, 0, 10) == 5 && numbers.integer_or("n", -1, 6, 10) == -1 && numbers.integer_or("big", -1, 0, 100) == -1);
  CHECK(numbers.integer_or("frac", -1, 0, 10) == -1 && numbers.integer_or("text", -1, 0, 10) == -1 && numbers.integer_or("absent", 3, 0, 10) == 3);
}

static void improv_packets() {
  const std::vector<std::uint8_t> state = improv::state_packet(improv::State::kReady);
  improv::Parser clean;
  std::optional<improv::Packet> got;
  for (std::size_t i = 0; i < state.size(); ++i) {
    const std::optional<improv::Packet> one = clean.feed(state[i]);
    if (one) {
      got = one;
      CHECK(i + 2 == state.size());   // whole when the checksum has arrived; the newline after it is dropped
    }
  }
  CHECK(got.has_value() && got->type == improv::Type::kCurrentState && got->data.size() == 1 && got->data[0] == 0x02 && clean.rejected() == 0);

  // a bit changed in the data or in the checksum: the packet is refused, and the next whole one is read
  std::vector<std::uint8_t> rpc_data{0x01, 0, 4, 's', 's', 'i', 'd', 4, 'p', 'a', 's', 's'};
  rpc_data[1] = static_cast<std::uint8_t>(rpc_data.size() - 2);
  const std::vector<std::uint8_t> rpc = improv::packet(improv::Type::kRpc, rpc_data);
  for (int round = 0; round < 200; ++round) {
    std::vector<std::uint8_t> bad = rpc;
    const std::size_t at = 9 + below(static_cast<std::uint32_t>(rpc.size() - 10));   // the data or the checksum, never the header nor the length
    bad[at] = static_cast<std::uint8_t>(bad[at] ^ (1u << below(8)));
    improv::Parser parser;
    int read = 0;
    for (std::uint8_t byte : bad) if (parser.feed(byte)) ++read;
    CHECK(read == 0 && parser.rejected() == 1);
    for (std::uint8_t byte : state) if (parser.feed(byte)) ++read;
    CHECK(read == 1);
  }

  // noise before a packet does not hide it
  for (int round = 0; round < 200; ++round) {
    improv::Parser parser;
    int read = 0;
    const std::string before = noise(below(80));
    for (char c : before) if (parser.feed(static_cast<std::uint8_t>(c))) ++read;
    CHECK(read == 0);
    for (std::uint8_t byte : state) if (parser.feed(byte)) ++read;
    CHECK(read == 1);
  }
  // noise alone: nothing is acted upon
  improv::Parser noisy;
  int accepted = 0;
  for (char c : noise(20000)) if (noisy.feed(static_cast<std::uint8_t>(c))) ++accepted;
  CHECK(accepted == 0);
  // a version this node does not speak is refused
  std::vector<std::uint8_t> other = state;
  other[6] = 0x02;
  improv::Parser version;
  int seen = 0;
  for (std::uint8_t byte : other) if (version.feed(byte)) ++seen;
  CHECK(seen == 0 && version.rejected() == 1);

  // the RPC: the one built is read back; shorter data is refused; random data never crashes and a Wi-Fi network that does not fit the standard is refused
  improv::Rpc out;
  CHECK(improv::parse_rpc(rpc_data, out) == improv::Error::kNone && out.command == improv::Command::kWifiSettings && out.ssid == "ssid" && out.password == "pass");
  for (std::size_t cut = 0; cut < rpc_data.size(); ++cut) {
    const std::vector<std::uint8_t> shorter(rpc_data.begin(), rpc_data.begin() + static_cast<std::ptrdiff_t>(cut));
    CHECK(improv::parse_rpc(shorter, out) == improv::Error::kInvalidRpc);
  }
  for (int round = 0; round < 3000; ++round) {
    std::vector<std::uint8_t> data(below(40));
    for (std::uint8_t& byte : data) byte = static_cast<std::uint8_t>(next_random());
    if (!data.empty() && below(2) == 0) data[0] = static_cast<std::uint8_t>(1 + below(6));
    if (data.size() >= 2 && below(2) == 0) data[1] = static_cast<std::uint8_t>(data.size() - 2);
    improv::Rpc parsed;
    if (improv::parse_rpc(data, parsed) == improv::Error::kNone && parsed.command == improv::Command::kWifiSettings) CHECK(!parsed.ssid.empty() && parsed.ssid.size() <= 32 && parsed.password.size() <= 63);
  }
  std::vector<std::uint8_t> long_name{0x01, 0, 33};
  long_name.insert(long_name.end(), 33, 'a');
  long_name.push_back(0);
  long_name[1] = static_cast<std::uint8_t>(long_name.size() - 2);
  CHECK(improv::parse_rpc(long_name, out) == improv::Error::kInvalidRpc);
  const std::vector<std::uint8_t> unknown{0x7f, 0};
  CHECK(improv::parse_rpc(unknown, out) == improv::Error::kUnknownRpc);
  // the page is told "joined" only with an address, and "failed" after the wait
  CHECK(improv::join_progress(true, true, 0, 1000) == improv::Join::kJoined && improv::join_progress(true, false, 500, 1000) == improv::Join::kWaiting && improv::join_progress(false, false, 1000, 1000) == improv::Join::kFailed);
  CHECK(improv::join_progress(true, true, 99999, 1000) == improv::Join::kJoined && improv::join_progress(true, false, 1000, 1000) == improv::Join::kFailed);
}

static void broker_addresses() {
  const broker_uri::HostPort plain = broker_uri::parse("mqtt://192.168.0.180");
  CHECK(plain.ok && plain.host == "192.168.0.180" && plain.port == 1883);
  CHECK(broker_uri::parse("mqtts://broker.local").port == 8883 && broker_uri::parse("wss://h/path").port == 443 && broker_uri::parse("ws://h").port == 80);
  CHECK(broker_uri::parse("mqtt://u:p@host:1884/x?y#z").host == "host" && broker_uri::parse("mqtt://u:p@host:1884/x?y#z").port == 1884);
  CHECK(broker_uri::parse("mqtt://[::1]:1884").host == "::1" && broker_uri::parse("mqtt://[::1]").port == 1883);
  for (const char* bad : {"", "mqtt:/h", "mqtt://", "mqtt://:1883", "mqtt://h:", "mqtt://h:0", "mqtt://h:65536", "mqtt://h:12x", "mqtt://h:123456", "http://h", "mqtt://[::1", "mqtt://[::1]x", "://h"}) CHECK(!broker_uri::parse(bad).ok);
  for (int round = 0; round < 3000; ++round) {
    const broker_uri::HostPort parsed = broker_uri::parse(mutate("mqtts://user:pw@broker.example:8884/path"));
    if (parsed.ok) CHECK(!parsed.host.empty() && parsed.port != 0);
  }
  for (int round = 0; round < 1000; ++round) {
    const broker_uri::HostPort parsed = broker_uri::parse(noise(below(60)));
    if (parsed.ok) CHECK(!parsed.host.empty() && parsed.port != 0);
  }
}

static void lost_gateway() {
  using linkwatch::Verdict;
  // the helper that judges a round
  CHECK(linkwatch::round_answered(0, 0) && linkwatch::round_answered(1, 3) && !linkwatch::round_answered(0, 3));

  // any run of answered and lost rounds, for many limits: a reconnect only after `limit` rounds in a row without a reply
  for (int limit = 1; limit <= 25; ++limit) {
    linkwatch::Watch watch(limit);
    const int effective = watch.limit();
    CHECK(effective == (limit > linkwatch::kMaxBadRounds ? linkwatch::kMaxBadRounds : limit));
    int silent = 0;
    bool ever_answered = false;
    for (int step = 0; step < 4000; ++step) {
      const std::uint32_t pick = below(100);
      if (pick == 0) {
        CHECK(watch.idle() == Verdict::kHealthy && watch.bad_rounds() == 0);
        silent = 0;
        ever_answered = false;
        continue;
      }
      // long outages and long good spells, as a link has
      const bool answered = (step / 37) % 3 == 0 ? pick < 98 : pick < 30;
      const Verdict verdict = watch.round(answered);
      if (answered) {
        CHECK(verdict == Verdict::kHealthy && watch.bad_rounds() == 0);
        silent = 0;
        ever_answered = true;
      } else {
        ++silent;
        if (verdict == Verdict::kReconnect) {
          CHECK(silent >= effective);
          silent = 0;
        } else if (silent >= effective) {
          silent = 0;   // the watch stood down: it counts from zero again
        }
      }
      CHECK(watch.bad_rounds() < effective);
      if (watch.stood_down()) CHECK(!watch.gateway_answers_pings());
      if (ever_answered) CHECK(!watch.stood_down());
    }
  }
  // a gateway that answers once and then goes away is an outage: it is reconnected every `limit` silent rounds, as often as it takes
  linkwatch::Watch outage(3);
  CHECK(outage.round(true) == Verdict::kHealthy && outage.gateway_answers_pings());
  int reconnects = 0;
  for (int i = 0; i < 30; ++i) if (outage.round(false) == Verdict::kReconnect) ++reconnects;
  CHECK(reconnects == 10 && !outage.stood_down());
  // a router that never answers a ping: one fresh association, then the watch stands down and stops insisting
  linkwatch::Watch mute(3);
  reconnects = 0;
  for (int i = 0; i < 60; ++i) if (mute.round(false) == Verdict::kReconnect) ++reconnects;
  CHECK(reconnects == linkwatch::kMaxFruitlessReconnects && mute.stood_down());
  CHECK(mute.round(true) == Verdict::kHealthy && !mute.stood_down() && mute.gateway_answers_pings());   // a reply ends it
  // no address: nothing to judge, and the memory of the last link is forgotten
  outage.round(false);
  CHECK(outage.idle() == Verdict::kHealthy && outage.bad_rounds() == 0 && !outage.gateway_answers_pings());
  linkwatch::Watch clamped(-5);
  CHECK(clamped.limit() == 1 && clamped.round(true) == Verdict::kHealthy && clamped.round(false) == Verdict::kReconnect);
  clamped.set_limit(1000);
  CHECK(clamped.limit() == linkwatch::kMaxBadRounds);
}

static void retry_waits() {
  // with no scatter the wait doubles up to its ceiling, and a good try starts again from the first
  retry::Backoff plain(500, 60000, 0);
  std::uint32_t last = 0;
  for (int i = 0; i < 40; ++i) {
    const std::uint32_t wait = plain.next(next_random());
    CHECK(wait >= last && wait <= 60000);
    if (i == 0) CHECK(wait == 500);
    if (i == 1) CHECK(wait == 1000);
    last = wait;
  }
  CHECK(last == 60000 && plain.failures() == 40);
  plain.reset();
  CHECK(plain.failures() == 0 && plain.next(1) == 500);
  // with scatter the wait stays inside the band and never above the ceiling nor below one
  for (std::uint32_t scatter : {5u, 20u, 50u, 90u, 200u}) {
    retry::Backoff scattered(1000, 30000, scatter);
    for (int i = 0; i < 200; ++i) {
      const std::uint32_t wait = scattered.next(next_random());
      CHECK(wait >= 1 && wait <= 30000);
      if (i == 0) {
        const std::uint32_t band = scatter > 90 ? 90 : scatter;
        CHECK(wait >= 1000 - 1000 * band / 100 && wait <= 1000 + 1000 * band / 100);
      }
      if (below(10) == 0) scattered.reset();
    }
  }
  // a ceiling below the first wait is the first wait
  retry::Backoff inverted(800, 100, 0);
  CHECK(inverted.next(0) == 800 && inverted.next(0) == 800);

  // the way back to the first broker: only when the time has come, and only if it answers
  linkwatch::BackupReturn back(100);
  int asked = 0;
  const auto answers = [&asked]() { ++asked; return true; };
  const auto silent = [&asked]() { ++asked; return false; };
  CHECK(back.connected_round(0, 10, answers) == linkwatch::BackStep::kStay && asked == 0);
  for (int i = 0; i < 9; ++i) CHECK(back.connected_round(1, 10, answers) == linkwatch::BackStep::kStay);
  CHECK(asked == 0 && back.rounds() == 9);
  CHECK(back.connected_round(1, 10, silent) == linkwatch::BackStep::kFirstStillDown && asked == 1 && back.rounds() == 0);
  for (int i = 0; i < 9; ++i) back.connected_round(2, 10, answers);
  CHECK(back.connected_round(2, 10, answers) == linkwatch::BackStep::kGoBack && asked == 2);
  for (int i = 0; i < 5; ++i) back.connected_round(1, 10, answers);
  back.reset();
  CHECK(back.rounds() == 0);
  for (int i = 0; i < 5; ++i) back.connected_round(1, 10, answers);
  CHECK(back.connected_round(0, 10, answers) == linkwatch::BackStep::kStay && back.rounds() == 0);   // back on the first broker: the count is dropped
  linkwatch::BackupReturn hasty(0);
  CHECK(hasty.connected_round(1, 0, answers) == linkwatch::BackStep::kGoBack);
}

int main() {
  bluetooth_frames();
  json_noise();
  improv_packets();
  broker_addresses();
  lost_gateway();
  retry_waits();
  std::printf("fuzz_link: %d checks, %d failed\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
