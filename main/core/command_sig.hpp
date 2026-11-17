// ARMOR-NODE-RADAR - the signature of the commands of the server (security option commands.require_sig of ARMOR-SERVER). No clock, no flash, no radio: the same code runs on the board and in the
// tests on a computer, against the vectors of ARMOR-COMMON (conformance/signatures/command.json).
//
// A signed command is the JSON of the command with "nonce" as its last field and then, as the very last, "sig": the payload ends with ,"sig":"<64 hex digits>"}. The signature is the
// HMAC-SHA256, in hex, of the text armor-command-v2| followed by the payload without that ending (a closing brace in its place), with the key of this node. The key of a node is made by the
// server from its master key and the id of the node and given to this node only; the master key never reaches a node. The check is done on the bytes as they arrived, so there is no JSON to
// rebuild. A node that holds a key refuses a command that does not verify, that repeats a nonce it has seen among the last ones, or, when its clock is valid, whose timestamp is not near its time.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "hmac_sha256.hpp"

namespace armor::command_sig {

inline constexpr std::string_view kPrefix = "armor-command-v2|";
inline constexpr std::string_view kEndingStart = ",\"sig\":\"";
inline constexpr std::size_t kSigHex = 64;
inline constexpr std::size_t kNonceHex = 16;
inline constexpr std::size_t kNonceMemory = 32;
/// How far the timestamp of a command may be from the time of the node, in milliseconds, when the node has a valid clock.
inline constexpr std::int64_t kFreshMs = 120'000;

enum class Verdict { kOk, kNoKey, kNoSignature, kBadSignature, kNoNonce, kReplay, kStale };

inline const char* to_text(Verdict v) {
  switch (v) {
    case Verdict::kOk: return "ok";
    case Verdict::kNoKey: return "no_key";
    case Verdict::kNoSignature: return "no_signature";
    case Verdict::kBadSignature: return "bad_signature";
    case Verdict::kNoNonce: return "no_nonce";
    case Verdict::kReplay: return "replay";
    case Verdict::kStale: return "stale";
  }
  return "?";
}

/// The nonces of the last commands that verified: a repeat is a replay. A ring, so the memory has a top.
class NonceCache {
 public:
  bool seen(std::string_view nonce) const {
    for (const std::string& item : items_) if (item == nonce) return true;
    return false;
  }
  void remember(std::string_view nonce) {
    items_[next_] = std::string(nonce);
    next_ = (next_ + 1) % items_.size();
  }
  void clear() { for (std::string& item : items_) item.clear(); next_ = 0; }

 private:
  std::array<std::string, kNonceMemory> items_{};
  std::size_t next_ = 0;
};

namespace detail {
inline bool is_lower_hex(std::string_view text) {
  for (const char c : text) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}
/// Equal in the same time whatever the first difference.
inline bool same(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  unsigned char diff = 0;
  for (std::size_t i = 0; i < a.size(); ++i) diff = static_cast<unsigned char>(diff | (static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i])));
  return diff == 0;
}
}  // namespace detail

/// A key of 64 lowercase hexadecimal digits.
inline bool key_is_valid(std::string_view key) { return key.size() == 64 && detail::is_lower_hex(key); }

/// The body (the payload with a closing brace in place of its ending) and the signature of a payload that ends with a signature; false for anything else.
inline bool split(std::string_view payload, std::string& body, std::string_view& sig) {
  const std::size_t ending = kEndingStart.size() + kSigHex + 2;   // ,"sig":" + 64 digits + "}
  if (payload.size() <= ending) return false;
  const std::size_t at = payload.size() - ending;
  if (payload.substr(at, kEndingStart.size()) != kEndingStart) return false;
  sig = payload.substr(at + kEndingStart.size(), kSigHex);
  if (!detail::is_lower_hex(sig) || payload.substr(payload.size() - 2) != "\"}") return false;
  body.assign(payload.substr(0, at));
  body.push_back('}');
  return true;
}

/// The nonce, which is the last field of the body: ,"nonce":"<16 hex digits>"} at its end.
inline bool nonce_of(std::string_view body, std::string& nonce) {
  constexpr std::string_view start = ",\"nonce\":\"";
  const std::size_t size = start.size() + kNonceHex + 2;
  if (body.size() <= size) return false;
  const std::size_t at = body.size() - size;
  if (body.substr(at, start.size()) != start || body.substr(body.size() - 2) != "\"}") return false;
  const std::string_view digits = body.substr(at + start.size(), kNonceHex);
  if (!detail::is_lower_hex(digits)) return false;
  nonce.assign(digits);
  return true;
}

/// The timestamp_ms of the body, or -1 when it has none (it is a number of digits right after the name).
inline std::int64_t timestamp_of(std::string_view body) {
  constexpr std::string_view name = "\"timestamp_ms\":";
  const std::size_t at = body.find(name);
  if (at == std::string_view::npos) return -1;
  std::int64_t value = 0;
  std::size_t i = at + name.size();
  const std::size_t first = i;
  while (i < body.size() && body[i] >= '0' && body[i] <= '9' && i - first < 15) value = value * 10 + (body[i++] - '0');
  return i == first ? -1 : value;
}

/// The signature the server would put on a body, in hex.
inline std::string signature_of(std::string_view key_hex, std::string_view body) {
  std::string key;
  if (!hash::from_hex(key_hex, key)) return std::string();
  std::string message(kPrefix);
  message.append(body);
  return hash::to_hex(hash::hmac_sha256(reinterpret_cast<const std::uint8_t*>(key.data()), key.size(), message));
}

/// Checks a command that arrived. `wall_ms` is the time of the node in milliseconds since 1970, or 0 when its clock is not valid (then the freshness is not checked and the nonces
/// alone stop a replay). The nonce is kept only once the signature is good, so a forged command cannot fill the memory.
/// `floor_ms` (0 = none) is the largest timestamp_ms of a command already taken, kept across a restart when the setting server.persist_replay_floor is on: a command that is not newer is stale, so a
/// signed command recorded before the restart cannot be played again once the memory of nonces is empty.
inline Verdict verify(std::string_view payload, std::string_view key_hex, NonceCache& cache, std::int64_t wall_ms, std::int64_t floor_ms = 0) {
  if (!key_is_valid(key_hex)) return Verdict::kNoKey;
  std::string body;
  std::string_view sig;
  if (!split(payload, body, sig)) return Verdict::kNoSignature;
  std::string nonce;
  if (!nonce_of(body, nonce)) return Verdict::kNoNonce;
  if (!detail::same(signature_of(key_hex, body), sig)) return Verdict::kBadSignature;
  if (cache.seen(nonce)) return Verdict::kReplay;
  if (wall_ms > 0) {
    const std::int64_t stamp = timestamp_of(body);
    const std::int64_t gap = stamp < 0 ? kFreshMs + 1 : (stamp > wall_ms ? stamp - wall_ms : wall_ms - stamp);
    if (gap > kFreshMs) return Verdict::kStale;
  }
  if (floor_ms > 0 && timestamp_of(body) <= floor_ms) return Verdict::kStale;
  cache.remember(nonce);
  return Verdict::kOk;
}

/// Checks an order of the node-control channel (armor/control/...), which the order gate reads without the nonce and the signature: on success `plain` is the order with both taken off.
inline Verdict verify_control(std::string_view payload, std::string_view key_hex, NonceCache& cache, std::int64_t wall_ms, std::string& plain) {
  const Verdict verdict = verify(payload, key_hex, cache, wall_ms);
  if (verdict != Verdict::kOk) return verdict;
  std::string body;
  std::string_view sig;
  std::string nonce;
  if (!split(payload, body, sig) || !nonce_of(body, nonce)) return Verdict::kNoSignature;   // cannot happen after a good verify
  constexpr std::size_t nonce_ending = 10 + kNonceHex + 2;   // ,"nonce":" + digits + "}
  body.resize(body.size() - nonce_ending);
  body.push_back('}');
  plain = body;
  return Verdict::kOk;
}

/// Whether an order of the node-control channel may be looked at. FAIL CLOSED: with `require_sig` on, an order without a good signature is refused, and so is every order when the node has no valid key.
/// With it off an unsigned order passes as it always did, but one that carries a signature is still checked (a forged one is refused, not read).
inline bool control_authorized(bool require_sig, std::string_view key_hex, std::string_view payload, NonceCache& cache, std::int64_t wall_ms, std::string& plain) {
  std::string body;
  std::string_view sig;
  const bool signed_form = split(payload, body, sig);
  if (!require_sig && !signed_form) { plain.assign(payload); return true; }
  return verify_control(payload, key_hex, cache, wall_ms, plain) == Verdict::kOk;
}

}  // namespace armor::command_sig
