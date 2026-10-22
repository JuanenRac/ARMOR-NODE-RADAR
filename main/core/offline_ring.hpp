// ARMOR-RADAR - the messages a node keeps while the broker cannot be reached, in flash, and sends when it comes back. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// An alarm event that happens while the broker is down must not be lost, nor lost because the node restarted meanwhile. The node keeps the last 30 of them (the oldest goes when the 31st comes),
// writes the whole ring to flash after each change, and sends them in order when the broker is back.
//
// How the flash is written safely: the ring is one blob with a magic, a generation number, its messages and a checksum, and there are TWO places (A and B) for it. A write goes to the place that
// is NOT the newest, and only when it has been written whole does it become the newest (the generation is higher). A power cut in the middle of a write leaves the other place intact, and at start
// the node takes the valid place with the higher generation; a place that fails its checksum is ignored (and the other one is used). Nothing is ever formatted or erased for it.
//
// Retained messages (the node's "online" birth, for instance) are not events: they are sent again by their own rule at each connection. If one is put in the ring all the same it REPLACES the one
// with the same key instead of being added, so a long outage never replays a hundred births.
#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace armor::offline {

constexpr std::size_t kMaxMessages = 30;
constexpr std::size_t kMaxTopic = 80;
constexpr std::size_t kMaxPayload = 240;
constexpr std::size_t kMaxBlob = 8192;

inline std::uint32_t crc32(const std::uint8_t* data, std::size_t length) {
  std::uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

struct Message {
  std::string topic;
  std::string payload;
  bool retained = false;
};

class Ring {
 public:
  // Adds a message. A message that does not fit the limits is refused (false). A retained message replaces the one with its topic.
  bool push(const std::string& topic, const std::string& payload, bool retained = false) {
    if (topic.empty() || topic.size() > kMaxTopic || payload.size() > kMaxPayload) return false;
    if (retained) for (Message& m : messages_) if (m.retained && m.topic == topic) { m.payload = payload; return true; }
    messages_.push_back(Message{topic, payload, retained});
    while (messages_.size() > kMaxMessages) { messages_.pop_front(); ++dropped_; }
    return true;
  }
  bool empty() const { return messages_.empty(); }
  std::size_t size() const { return messages_.size(); }
  std::uint32_t dropped() const { return dropped_; }
  const Message& front() const { return messages_.front(); }
  const Message& at(std::size_t i) const { return messages_[i]; }
  void pop_front() { if (!messages_.empty()) messages_.pop_front(); }
  void clear() { messages_.clear(); }

  // The ring as bytes: "ARB1", generation (4), count (2), then per message topic length (1), topic, payload length (2), payload, retained (1); then the CRC-32 of everything before it.
  std::string serialize(std::uint32_t generation) const {
    std::string out = "ARB1";
    put32(out, generation);
    put16(out, static_cast<std::uint16_t>(messages_.size()));
    for (const Message& m : messages_) {
      out += static_cast<char>(m.topic.size());
      out += m.topic;
      put16(out, static_cast<std::uint16_t>(m.payload.size()));
      out += m.payload;
      out += static_cast<char>(m.retained ? 1 : 0);
    }
    put32(out, crc32(reinterpret_cast<const std::uint8_t*>(out.data()), out.size()));
    return out;
  }

  // Reads a blob; false (and the ring untouched) when it is not a whole, valid one.
  bool parse(const std::string& blob, std::uint32_t& generation) {
    if (blob.size() < 4 + 4 + 2 + 4 || blob.size() > kMaxBlob || blob.compare(0, 4, "ARB1") != 0) return false;
    const std::uint8_t* d = reinterpret_cast<const std::uint8_t*>(blob.data());
    const std::uint32_t stored = get32(d + blob.size() - 4);
    if (crc32(d, blob.size() - 4) != stored) return false;
    std::size_t at = 4;
    const std::uint32_t gen = get32(d + at); at += 4;
    const std::size_t count = get16(d + at); at += 2;
    if (count > kMaxMessages) return false;
    std::deque<Message> read;
    const std::size_t end = blob.size() - 4;
    for (std::size_t i = 0; i < count; ++i) {
      if (at + 1 > end) return false;
      const std::size_t topic_length = d[at++];
      if (topic_length == 0 || topic_length > kMaxTopic || at + topic_length + 2 > end) return false;
      Message m;
      m.topic.assign(blob, at, topic_length); at += topic_length;
      const std::size_t payload_length = get16(d + at); at += 2;
      if (payload_length > kMaxPayload || at + payload_length + 1 > end) return false;
      m.payload.assign(blob, at, payload_length); at += payload_length;
      m.retained = d[at++] != 0;
      read.push_back(std::move(m));
    }
    if (at != end) return false;
    messages_ = std::move(read);
    generation = gen;
    return true;
  }

 private:
  static void put16(std::string& s, std::uint16_t v) { s += static_cast<char>(v & 0xFF); s += static_cast<char>(v >> 8); }
  static void put32(std::string& s, std::uint32_t v) { for (int i = 0; i < 4; ++i) s += static_cast<char>((v >> (8 * i)) & 0xFF); }
  static std::uint16_t get16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }
  static std::uint32_t get32(const std::uint8_t* p) { return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) | (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24); }

  std::deque<Message> messages_;
  std::uint32_t dropped_ = 0;
};

// Where the two places are: the firmware keeps them in two blobs of the settings store; the tests in two strings that can fail halfway.
class Storage {
 public:
  virtual ~Storage() = default;
  virtual bool read(int place, std::string& out) = 0;          // place 0 or 1; false when there is nothing there
  virtual bool write(int place, const std::string& blob) = 0;  // false when the write failed
};

class DurableRing {
 public:
  explicit DurableRing(Storage& storage) : storage_(storage) {}

  // At start: the valid place with the higher generation (the other one is ignored). Returns how many messages came back.
  std::size_t load() {
    std::string blob;
    Ring candidate[2];
    std::uint32_t generation[2] = {0, 0};
    bool valid[2] = {false, false};
    for (int place = 0; place < 2; ++place) valid[place] = storage_.read(place, blob) && candidate[place].parse(blob, generation[place]);
    if (!valid[0] && !valid[1]) { ring_ = Ring{}; generation_ = 0; newest_ = 1; return 0; }
    const int pick = valid[0] && valid[1] ? (generation[1] > generation[0] ? 1 : 0) : (valid[0] ? 0 : 1);   // equal generations cannot happen after a clean write; place 0 wins
    ring_ = candidate[pick];
    generation_ = generation[pick];
    newest_ = pick;
    return ring_.size();
  }

  // `save_now` false: the message is kept in memory and written with the next flush(), so a burst of events is one write to the flash and not thirty.
  bool push(const std::string& topic, const std::string& payload, bool retained = false, bool save_now = true) {
    if (!ring_.push(topic, payload, retained)) return false;
    if (save_now) save(); else dirty_ = true;
    return true;
  }
  void flush() { if (dirty_) save(); }
  bool dirty() const { return dirty_; }

  // Sends the messages in order through `send` (true when the message was handed to the broker); stops at the first refusal and keeps what was not sent. The flash is written once at the end.
  template <typename Send>
  std::size_t drain(Send&& send) {
    std::size_t sent = 0;
    while (!ring_.empty()) {
      if (!send(ring_.front())) break;
      ring_.pop_front();
      ++sent;
    }
    if (sent > 0) save();
    return sent;
  }

  std::size_t size() const { return ring_.size(); }
  std::uint32_t dropped() const { return ring_.dropped(); }
  std::uint32_t generation() const { return generation_; }
  std::uint32_t failed_writes() const { return failed_writes_; }
  const Ring& ring() const { return ring_; }

 private:
  void save() {
    const int place = newest_ == 0 ? 1 : 0;   // the one that is not the newest
    const std::string blob = ring_.serialize(generation_ + 1);
    if (!storage_.write(place, blob)) { ++failed_writes_; dirty_ = true; return; }   // the newest place is untouched: nothing was lost that was already safe; the next flush tries again
    ++generation_;
    newest_ = place;
    dirty_ = false;
  }

  Storage& storage_;
  Ring ring_;
  std::uint32_t generation_ = 0;
  int newest_ = 1;   // so that the first write goes to place 0
  std::uint32_t failed_writes_ = 0;
  bool dirty_ = false;
};

}  // namespace armor::offline
