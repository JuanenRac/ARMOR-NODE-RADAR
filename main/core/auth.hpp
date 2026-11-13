// ARMOR-NODE-RADAR - who may use the panel: users, roles, sessions and the throttle on wrong passwords.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Passwords are stored only as a salted hash. The hash function itself (PBKDF2 with HMAC-SHA256 on the board, a stand-in in the tests)
// is passed in, so all the rules around it - who exists, who is admin, when a login is refused, when a session ends - are tested on a
// computer. A node that has no user yet is in "setup": the first administrator is created with a code that is only shown on the node's
// own console, so nobody on the network can claim a fresh node.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "json.hpp"

namespace armor::auth {

enum class Role { kViewer, kAdmin };
constexpr std::size_t kMaxUsers = 4;
constexpr std::size_t kMinPassword = 8, kMaxPassword = 64;

inline const char* to_text(Role role) { return role == Role::kAdmin ? "admin" : "viewer"; }

struct User {
  std::string name;
  Role role = Role::kViewer;
  std::string salt;  // hexadecimal
  std::string hash;  // hexadecimal
};

// Computes the stored hash of a password for a salt (both text); the same inputs always give the same output.
using Hasher = std::function<std::string(std::string_view password, std::string_view salt)>;

inline bool valid_user_name(std::string_view name) {
  if (name.size() < 3 || name.size() > 32) return false;
  for (const char c : name) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-')) return false;
  return true;
}
inline bool valid_password(std::string_view password) {
  if (password.size() < kMinPassword || password.size() > kMaxPassword) return false;
  for (const char c : password) if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f) return false;
  return true;
}

// The key under which the wrong passwords of one person are counted (FNV-1a of the lower-case name, never 0): a guess against an account is slowed down wherever it comes from.
inline std::uint32_t user_key(std::string_view name) {
  std::uint32_t h = 2166136261u;
  for (const char c : name) { h ^= static_cast<std::uint8_t>(c >= 'A' && c <= 'Z' ? c + 32 : c); h *= 16777619u; }
  return h == 0 ? 1u : h;
}

// Compares two texts in a time that does not depend on where they first differ.
inline bool same_text(std::string_view a, std::string_view b) {
  unsigned char difference = static_cast<unsigned char>(a.size() != b.size());
  const std::size_t n = std::min(a.size(), b.size());
  for (std::size_t i = 0; i < n; ++i) difference = static_cast<unsigned char>(difference | (static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i])));
  return difference == 0;
}

// Text made from random bytes (the caller supplies them from the hardware generator).
inline std::string to_hex(const std::uint8_t* bytes, std::size_t length) {
  static const char digits[] = "0123456789abcdef";
  std::string out;
  for (std::size_t i = 0; i < length; ++i) { out += digits[bytes[i] >> 4]; out += digits[bytes[i] & 15]; }
  return out;
}
// The setup code: capital letters and digits without the ones that look alike (0 O 1 I L). 31 symbols, so the byte is reduced with a small, harmless bias.
inline std::string setup_code_from(const std::uint8_t* bytes, std::size_t length) {
  static const char alphabet[] = "ABCDEFGHJKMNPQRSTUVWXYZ23456789";
  std::string out;
  for (std::size_t i = 0; i < length; ++i) out += alphabet[bytes[i] % 31];
  return out;
}

// The identity of a node is its Wi-Fi station MAC address, on both boards and whatever cable is plugged in: the name, the setup code and the code derived from the fleet secret all come from
// these twelve digits. (The MAC the Ethernet port shows is another address and is only displayed, never used to derive anything.)
inline std::string identity_mac_text(const std::uint8_t* wifi_station_mac) { return to_hex(wifi_station_mac, 6); }

// HMAC-SHA256 as the board has it (mbedTLS) or as a stand-in in the tests: key, message, 32 bytes out; false when it failed.
using HmacSha256 = std::function<bool(std::string_view key, std::string_view message, std::uint8_t* digest)>;
constexpr std::size_t kMinFleetSecret = 16;   // a shorter secret is not a secret: it derives nothing
constexpr std::size_t kMinSetupCode = 8;

// The setup code the fleet secret gives to the node with this Wi-Fi MAC: ten symbols of the HMAC of the twelve hexadecimal digits. Empty when the secret is too short or the HMAC failed.
inline std::string setup_code_from_fleet_secret(std::string_view fleet_secret, const std::uint8_t* wifi_station_mac, const HmacSha256& hmac) {
  if (fleet_secret.size() < kMinFleetSecret || !hmac) return std::string();
  std::uint8_t digest[32];
  if (!hmac(fleet_secret, identity_mac_text(wifi_station_mac), digest)) return std::string();
  return setup_code_from(digest, 10);
}

// Where the board's setup code comes from, in this order: the fleet secret (the owner can always work it out again), the code built into the image (written in capitals, as the panel's form
// writes it), the code kept in flash, and last a random one. `stored` is what was read from flash ("" when nothing); `random_bytes` gives eight bytes; `keep` says that the code must be written to flash.
struct SetupCodeChoice { std::string code; bool keep = false; };
inline SetupCodeChoice choose_setup_code(std::string_view fleet_secret, std::string_view built_in, std::string_view stored, const std::uint8_t* wifi_station_mac, const HmacSha256& hmac,
                                         const std::function<void(std::uint8_t*, std::size_t)>& random_bytes) {
  SetupCodeChoice out;
  out.code = setup_code_from_fleet_secret(fleet_secret, wifi_station_mac, hmac);
  if (!out.code.empty()) return out;
  if (built_in.size() >= kMinSetupCode) {
    out.code.assign(built_in);
    for (char& c : out.code) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return out;
  }
  if (stored.size() >= kMinSetupCode) { out.code.assign(stored); return out; }
  std::uint8_t bytes[8] = {};
  if (random_bytes) random_bytes(bytes, sizeof bytes);
  out.code = setup_code_from(bytes, sizeof bytes);
  out.keep = true;
  return out;
}

// The passkey of Bluetooth pairing: six digits that depend on this node's setup code and on nothing else, so the node needs to keep nothing more and whoever has the label or the USB
// console has the PIN. The node shows it where it shows the setup code; the phone asks for it when it pairs. It is a one-way mix: the PIN does not give the setup code back.
inline std::uint32_t ble_passkey_from(const std::string& setup_code) {
  std::uint64_t h = 1469598103934665603ULL;
  const std::string salted = "armor-ble-passkey:" + setup_code;
  for (const char c : salted) { h ^= static_cast<std::uint8_t>(c); h *= 1099511628211ULL; }
  h ^= h >> 29; h *= 0xbf58476d1ce4e5b9ULL; h ^= h >> 32;
  return static_cast<std::uint32_t>(h % 1000000ULL);
}

// The PIN of a pairing in the mode `random_display`: six digits made from a random number of the radio's generator, never from the setup code, so the label does not give it away.
// Rejection sampling keeps every digit equally likely.
template <typename Random32>
inline std::uint32_t ble_random_passkey(Random32 random32) {
  constexpr std::uint32_t kLimit = 4294000000u;   // the largest multiple of 1,000,000 that fits in 32 bits
  std::uint32_t value = random32();
  for (int tries = 0; value >= kLimit && tries < 8; ++tries) value = random32();
  return (value >= kLimit ? value - kLimit : value) % 1000000u;
}

enum class Result { kOk, kInvalidName, kWeakPassword, kExists, kNotFound, kTooMany, kLastAdmin, kWrongPassword, kStorage };   // kStorage: the change could not be written to flash, so it was not made

class UserStore {
 public:
  bool empty() const { return users_.empty(); }
  const std::vector<User>& users() const { return users_; }
  const User* find(std::string_view name) const {
    for (const User& user : users_) if (user.name == name) return &user;
    return nullptr;
  }
  std::size_t admins() const { return static_cast<std::size_t>(std::count_if(users_.begin(), users_.end(), [](const User& u) { return u.role == Role::kAdmin; })); }

  Result add(std::string_view name, std::string_view password, Role role, std::string_view salt, const Hasher& hasher) {
    if (!valid_user_name(name)) return Result::kInvalidName;
    if (!valid_password(password)) return Result::kWeakPassword;
    if (find(name) != nullptr) return Result::kExists;
    if (users_.size() >= kMaxUsers) return Result::kTooMany;
    users_.push_back({std::string(name), role, std::string(salt), hasher(password, salt)});
    return Result::kOk;
  }
  // The role of the user when the password is right.
  bool verify(std::string_view name, std::string_view password, const Hasher& hasher, Role& role) const {
    const User* user = find(name);
    // An unknown name still costs one hash, so the answer time does not say which names exist.
    const std::string computed = hasher(password, user != nullptr ? std::string_view(user->salt) : std::string_view("00000000000000000000000000000000"));
    if (user == nullptr || !same_text(computed, user->hash)) return false;
    role = user->role;
    return true;
  }
  Result set_password(std::string_view name, std::string_view password, std::string_view salt, const Hasher& hasher) {
    User* user = find_mutable(name);
    if (user == nullptr) return Result::kNotFound;
    if (!valid_password(password)) return Result::kWeakPassword;
    user->salt = std::string(salt);
    user->hash = hasher(password, salt);
    return Result::kOk;
  }
  Result set_role(std::string_view name, Role role) {
    User* user = find_mutable(name);
    if (user == nullptr) return Result::kNotFound;
    if (user->role == Role::kAdmin && role != Role::kAdmin && admins() == 1) return Result::kLastAdmin;
    user->role = role;
    return Result::kOk;
  }
  Result remove(std::string_view name) {
    const User* user = find(name);
    if (user == nullptr) return Result::kNotFound;
    if (user->role == Role::kAdmin && admins() == 1) return Result::kLastAdmin;
    users_.erase(std::remove_if(users_.begin(), users_.end(), [&](const User& u) { return u.name == name; }), users_.end());
    return Result::kOk;
  }
  void clear() { users_.clear(); }

  // Storage: {"users":[{"name":"admin","role":"admin","salt":"..","hash":".."}]}
  std::string to_json() const {
    json::Writer w;
    w.begin_object().key("users").begin_array();
    for (const User& user : users_) w.begin_object().field("name", user.name).field("role", to_text(user.role)).field("salt", user.salt).field("hash", user.hash).end_object();
    w.end_array().end_object();
    return w.str();
  }
  bool from_json(std::string_view text) {
    json::Value document;
    if (!json::parse(text, document)) return false;
    const json::Value* list = document.get("users");
    if (list == nullptr || !list->is_array() || list->items.size() > kMaxUsers) return false;
    std::vector<User> loaded;
    for (const json::Value& item : list->items) {
      User user;
      user.name = item.string_or("name", "");
      const std::string role = item.string_or("role", "");
      user.salt = item.string_or("salt", "");
      user.hash = item.string_or("hash", "");
      if (!valid_user_name(user.name) || (role != "admin" && role != "viewer") || user.salt.empty() || user.hash.empty() || find_in(loaded, user.name)) return false;
      user.role = role == "admin" ? Role::kAdmin : Role::kViewer;
      loaded.push_back(std::move(user));
    }
    users_ = std::move(loaded);
    return true;
  }

 private:
  static bool find_in(const std::vector<User>& list, std::string_view name) {
    return std::any_of(list.begin(), list.end(), [&](const User& u) { return u.name == name; });
  }
  User* find_mutable(std::string_view name) {
    for (User& user : users_) if (user.name == name) return &user;
    return nullptr;
  }
  std::vector<User> users_;
};

// ---- sessions ------------------------------------------------------------------------------------------------------------------

constexpr std::size_t kMaxSessions = 6;
constexpr std::uint64_t kSessionIdleMs = 30ULL * 60ULL * 1000ULL;
// "Remember me" at login: a session that survives the browser closing (a long-lived cookie) also gets a long idle allowance here, or
// the cookie would outlive the server-side session it names and the panel would still ask to sign in again after 30 minutes away.
constexpr std::uint64_t kRememberedIdleMs = 30ULL * 24ULL * 60ULL * 60ULL * 1000ULL;

struct Session {
  std::string token;  // random, hexadecimal; created by the caller
  std::string user;
  Role role = Role::kViewer;
  std::uint64_t last_seen_ms = 0;
  bool used = false;
  bool remembered = false;
};

class SessionTable {
 public:
  // Starts a session; when the table is full the one idle the longest is replaced.
  void create(std::string token, std::string user, Role role, std::uint64_t now_ms, bool remembered = false) {
    Session* slot = nullptr;
    // A free place first, then one whose owner has been idle longer than ITS OWN allowance (a "remember me" session lives thirty days, not thirty minutes: it used to be taken for
    // free after thirty minutes, so logging in from a laptop threw the phone out), then the one idle the longest.
    for (Session& session : sessions_) if (session.used && same_text(session.token, token)) { slot = &session; break; }   // the same token starts over, never twice
    if (slot == nullptr) for (Session& session : sessions_) if (!session.used) { slot = &session; break; }
    if (slot == nullptr) for (Session& session : sessions_) if (now_ms - session.last_seen_ms > (session.remembered ? kRememberedIdleMs : kSessionIdleMs)) { slot = &session; break; }
    if (slot == nullptr) slot = &*std::min_element(sessions_.begin(), sessions_.end(), [](const Session& a, const Session& b) { return a.last_seen_ms < b.last_seen_ms; });
    *slot = {std::move(token), std::move(user), role, now_ms, true, remembered};
  }
  // The session for a token, refreshed; nullptr when there is none or it has been idle too long.
  const Session* touch(std::string_view token, std::uint64_t now_ms) {
    if (token.size() < 32) return nullptr;
    for (Session& session : sessions_) {
      if (!session.used || !same_text(session.token, token)) continue;
      const std::uint64_t allowed = session.remembered ? kRememberedIdleMs : kSessionIdleMs;
      if (now_ms - session.last_seen_ms > allowed) { session = Session{}; return nullptr; }
      session.last_seen_ms = now_ms;
      return &session;
    }
    return nullptr;
  }
  // Whether a token still has a live session, without refreshing it: a long-lived connection (a map that is pushed to a browser) asks this to know when to let go.
  bool alive(std::string_view token, std::uint64_t now_ms) const {
    if (token.size() < 32) return false;
    for (const Session& session : sessions_) {
      if (!session.used || !same_text(session.token, token)) continue;
      return now_ms - session.last_seen_ms <= (session.remembered ? kRememberedIdleMs : kSessionIdleMs);
    }
    return false;
  }
  void end(std::string_view token) {
    for (Session& session : sessions_) if (session.used && same_text(session.token, token)) session = Session{};
  }
  // Ends every session of a user (after a password change or a removal).
  void end_user(std::string_view user) {
    for (Session& session : sessions_) if (session.used && session.user == user) session = Session{};
  }
  void clear() { for (Session& session : sessions_) session = Session{}; }

 private:
  std::array<Session, kMaxSessions> sessions_{};
};

// ---- the throttle --------------------------------------------------------------------------------------------------------------

// After five wrong passwords from one address the address is refused for 30 seconds, then for twice as long each time (at most five
// minutes); a right password forgives. The table holds thirty-two sources (addresses, and the people the failures are counted for). A blocked source is never pushed out to make room for another (with more sources than places, the block
// of the first would have been shaken off): when every place holds an address that is blocked right now, an address the table does not know waits too.
class LoginThrottle {
 public:
  static constexpr std::size_t kThrottlePlaces = 32;
  bool allowed(std::uint32_t source, std::uint64_t now_ms) const {
    const Entry* entry = find(source);
    if (entry != nullptr) return now_ms >= entry->blocked_until_ms;
    return has_place(now_ms);
  }
  // Seconds to wait, for the message (0 when allowed).
  unsigned wait_s(std::uint32_t source, std::uint64_t now_ms) const {
    const Entry* entry = find(source);
    if (entry != nullptr) return now_ms >= entry->blocked_until_ms ? 0 : static_cast<unsigned>((entry->blocked_until_ms - now_ms + 999) / 1000);
    if (has_place(now_ms)) return 0;
    std::uint64_t soonest = ~0ULL;
    for (const Entry& held : entries_) if (held.used && held.blocked_until_ms < soonest) soonest = held.blocked_until_ms;
    return static_cast<unsigned>((soonest - now_ms + 999) / 1000);
  }
  void failure(std::uint32_t source, std::uint64_t now_ms) {
    Entry* entry = obtain(source, now_ms);
    if (entry == nullptr) return;   // every place holds a blocked address: this one is not let in to try (allowed() said so), so there is nothing to count
    count_failure(*entry, now_ms, 5);
  }
  void success(std::uint32_t source) {
    for (Entry& entry : entries_) if (entry.used && entry.source == source) entry = Entry{};
  }

  // The failures of ALL the sources together, in a place of their own that no source can push out of the table: where one source stands for many (every phone in range of a node's
  // Bluetooth), it blocks everybody after `limit` wrong passwords in a row, from anyone, with the same growing wait.
  bool shared_allowed(std::uint64_t now_ms) const { return now_ms >= shared_.blocked_until_ms; }
  unsigned shared_wait_s(std::uint64_t now_ms) const { return now_ms >= shared_.blocked_until_ms ? 0 : static_cast<unsigned>((shared_.blocked_until_ms - now_ms + 999) / 1000); }
  void shared_failure(std::uint64_t now_ms, unsigned limit) { count_failure(shared_, now_ms, limit); }
  void shared_success() { shared_ = Entry{}; }

  // A limit on the wrong passwords of ALL the sources together, per minute: however many addresses and names are tried, no more than kMinuteFailures wrong attempts are let through in
  // any minute (each one costs the node a third of a second of password hashing). Only failures are counted, so people who sign in correctly are not what fills it.
  // The number is a setting of the node (set_minute_limit): 30 unless it says otherwise, 0 turns this limit off (the limits of each address and of each person stay), at most kMinuteFailuresMax.
  static constexpr unsigned kMinuteFailures = 30;
  static constexpr unsigned kMinuteFailuresMax = 120;
  static constexpr std::uint64_t kMinuteMs = 60000;
  void set_minute_limit(unsigned limit) { minute_limit_ = limit > kMinuteFailuresMax ? kMinuteFailuresMax : limit; }
  unsigned minute_limit() const { return minute_limit_; }
  bool minute_allowed(std::uint64_t now_ms) const { return minute_wait_s(now_ms) == 0; }
  unsigned minute_wait_s(std::uint64_t now_ms) const {
    if (minute_limit_ == 0) return 0;
    unsigned inside = 0;
    for (unsigned back = 1; back <= minute_count_; ++back) {   // from the newest failure to the oldest still remembered
      const std::uint64_t at = minute_recent_[(minute_head_ + kMinuteFailuresMax - back) % kMinuteFailuresMax];
      if (now_ms >= at + kMinuteMs) break;                       // this one and every older one have left the minute
      if (++inside == minute_limit_) return static_cast<unsigned>((at + kMinuteMs - now_ms + 999) / 1000);   // the limit-th newest frees the place when it leaves
    }
    return 0;
  }
  void minute_failure(std::uint64_t now_ms) {
    minute_recent_[minute_head_] = now_ms;
    minute_head_ = (minute_head_ + 1) % kMinuteFailuresMax;
    if (minute_count_ < kMinuteFailuresMax) ++minute_count_;
  }

 private:
  struct Entry {
    bool used = false;
    std::uint32_t source = 0;
    unsigned failures = 0;
    std::uint64_t blocked_until_ms = 0, last_ms = 0;
  };
  static void count_failure(Entry& entry, std::uint64_t now_ms, unsigned limit) {
    entry.last_ms = now_ms;
    if (++entry.failures >= limit) {
      const unsigned doublings = std::min(entry.failures - limit, 4u);
      entry.blocked_until_ms = now_ms + std::min<std::uint64_t>(30000ULL << doublings, 300000ULL);
    }
  }
  const Entry* find(std::uint32_t source) const {
    for (const Entry& entry : entries_) if (entry.used && entry.source == source) return &entry;
    return nullptr;
  }
  // Whether a place is free, or held by an address whose block has ended (that one can go).
  bool has_place(std::uint64_t now_ms) const {
    for (const Entry& entry : entries_) if (!entry.used || now_ms >= entry.blocked_until_ms) return true;
    return false;
  }
  Entry* obtain(std::uint32_t source, std::uint64_t now_ms) {
    for (Entry& entry : entries_) if (entry.used && entry.source == source) return &entry;
    Entry* slot = nullptr;
    for (Entry& entry : entries_) if (!entry.used) { slot = &entry; break; }
    if (slot == nullptr) {
      for (Entry& entry : entries_) if (now_ms >= entry.blocked_until_ms && (slot == nullptr || entry.last_ms < slot->last_ms)) slot = &entry;   // the one idle the longest of those not blocked
    }
    if (slot == nullptr) return nullptr;
    *slot = Entry{};
    slot->used = true;
    slot->source = source;
    slot->last_ms = now_ms;
    return slot;
  }
  std::array<Entry, kThrottlePlaces> entries_{};
  Entry shared_{};
  std::array<std::uint64_t, kMinuteFailuresMax> minute_recent_{};
  unsigned minute_head_ = 0, minute_count_ = 0, minute_limit_ = kMinuteFailures;
};

}  // namespace armor::auth
