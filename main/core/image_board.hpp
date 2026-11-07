// ARMOR-NODE-RADAR - which board a firmware image was built for, read from the image itself. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Both boards (s3-wifi and s3-eth) run the same project, so the name inside an image is the same for both and the name alone cannot tell them apart. An image for the wrong board is
// not a harmless mistake: a Wi-Fi image on the Ethernet board comes up with no cable driver, opens the rescue access point and, because that counts as reachable, is confirmed, so the
// node is out of the wired network for good. Every image therefore carries a small mark with the board it was built for, and an upload of an image is checked against it.
//
// The mark is a fixed 8-byte signature, the length of the board's name, the name, a zero and a check byte. The signature is computed (never written as a run of constants) so the
// code that looks for it does not keep a second copy of it, and anything that looks like the signature but is not followed by a well-formed mark is ignored: a copy of the signature
// that happens to sit in the code cannot make an image look as if it were for another board. The scanner is fed the image in the pieces it arrives in, so the image is never held whole.
//
// What is done with the verdict is the operator's to choose (security.ota_check_board and security.ota_require_board_mark in the settings); both are on by default.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace armor::image_board {

constexpr std::size_t kSignatureBytes = 8;
constexpr std::size_t kMaxBoardChars = 16;
constexpr std::size_t kMaxDistinctMarks = 4;   // what is remembered of an image that carries many marks

// Byte i of the signature. All eight differ, so a failed match can restart at the first byte or the second without losing a real signature.
constexpr std::uint8_t signature_byte(std::size_t i) { return static_cast<std::uint8_t>(0xA7u ^ (i * 0x2Du) ^ (i << 5)); }

constexpr bool signature_bytes_differ() {
  for (std::size_t a = 0; a < kSignatureBytes; ++a)
    for (std::size_t b = a + 1; b < kSignatureBytes; ++b)
      if (signature_byte(a) == signature_byte(b)) return false;
  return true;
}
static_assert(signature_bytes_differ(), "the signature must not repeat a byte");

constexpr bool board_char_ok(unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; }

// The mark of a board; `id` is the board's name as a string literal ("s3-eth").
template <std::size_t M>
constexpr std::array<std::uint8_t, kSignatureBytes + 1 + (M - 1) + 2> make_mark(const char (&id)[M]) {
  static_assert(M >= 2 && M - 1 <= kMaxBoardChars, "the name of a board has 1 to 16 characters");
  std::array<std::uint8_t, kSignatureBytes + 1 + (M - 1) + 2> mark{};
  for (std::size_t i = 0; i < kSignatureBytes; ++i) mark[i] = signature_byte(i);
  mark[kSignatureBytes] = static_cast<std::uint8_t>(M - 1);
  unsigned sum = 0;
  for (std::size_t i = 0; i + 1 < M; ++i) {
    mark[kSignatureBytes + 1 + i] = static_cast<std::uint8_t>(id[i]);
    sum += static_cast<unsigned char>(id[i]);
  }
  mark[kSignatureBytes + 1 + (M - 1)] = 0;
  mark[kSignatureBytes + 1 + (M - 1) + 1] = static_cast<std::uint8_t>((sum ^ 0x5Cu) & 0xFFu);
  return mark;
}

// The mark of the board this firmware is built for. `keep()` is called once at start so the linker has a reason not to drop it.
#if defined(ARMOR_BOARD_S3_ETH)
inline constexpr auto kOwnMark = make_mark("s3-eth");
#else
inline constexpr auto kOwnMark = make_mark("s3-wifi");
#endif
inline std::uint8_t keep() {
  static const volatile std::uint8_t* const mark = kOwnMark.data();
  return mark[kSignatureBytes];
}

enum class Verdict {
  kMatch,        // every mark in the image is this board's, and there is at least one
  kOtherBoard,   // a mark names another board
  kNoMark,       // no mark at all: an image made before the marks existed, or by something else
};

// What an upload does about the verdict.
struct Policy {
  bool check_board = true;          // refuse an image that names another board
  bool require_mark = true;         // refuse an image with no mark at all (an old image); off: it is accepted
};
enum class Decision { kAccept, kWrongBoard, kNoBoardMark };

inline Decision decide(Verdict verdict, const Policy& policy) {
  if (verdict == Verdict::kOtherBoard) return policy.check_board ? Decision::kWrongBoard : Decision::kAccept;
  if (verdict == Verdict::kNoMark) return policy.check_board && policy.require_mark ? Decision::kNoBoardMark : Decision::kAccept;
  return Decision::kAccept;
}
inline const char* decision_code(Decision decision) {
  return decision == Decision::kWrongBoard ? "wrong_board" : decision == Decision::kNoBoardMark ? "no_board_mark" : "";
}

// Looks for the marks in a stream of bytes. One byte of state per step, so it costs nothing next to writing the flash.
class Scanner {
 public:
  void feed(const std::uint8_t* data, std::size_t length) {
    for (std::size_t i = 0; i < length; ++i) step(data[i]);
  }
  void feed(std::string_view data) { feed(reinterpret_cast<const std::uint8_t*>(data.data()), data.size()); }

  // The boards named by the well-formed marks seen so far (each once).
  const std::vector<std::string>& boards() const { return boards_; }
  std::size_t marks_seen() const { return marks_; }

  Verdict verdict(std::string_view board) const {
    if (boards_.empty()) return Verdict::kNoMark;
    for (const std::string& seen : boards_) if (seen != board) return Verdict::kOtherBoard;
    return Verdict::kMatch;
  }

 private:
  enum class Phase { kSignature, kLength, kName, kZero, kCheck };

  void step(std::uint8_t byte) {
    switch (phase_) {
      case Phase::kSignature:
        if (byte == signature_byte(matched_)) { if (++matched_ == kSignatureBytes) { phase_ = Phase::kLength; matched_ = 0; } }
        else matched_ = byte == signature_byte(0) ? 1 : 0;
        return;
      case Phase::kLength:
        if (byte == 0 || byte > kMaxBoardChars) { restart(byte); return; }
        length_ = byte; name_.clear(); sum_ = 0; phase_ = Phase::kName;
        return;
      case Phase::kName:
        if (!board_char_ok(byte)) { restart(byte); return; }
        name_.push_back(static_cast<char>(byte)); sum_ += byte;
        if (name_.size() == length_) phase_ = Phase::kZero;
        return;
      case Phase::kZero:
        if (byte != 0) { restart(byte); return; }
        phase_ = Phase::kCheck;
        return;
      case Phase::kCheck:
        if (byte == static_cast<std::uint8_t>((sum_ ^ 0x5Cu) & 0xFFu)) record();
        restart(byte);
        return;
    }
  }
  // Back to looking for a signature; the byte that broke the mark may itself start one.
  void restart(std::uint8_t byte) {
    phase_ = Phase::kSignature;
    matched_ = byte == signature_byte(0) ? 1 : 0;
  }
  void record() {
    ++marks_;
    for (const std::string& seen : boards_) if (seen == name_) return;
    if (boards_.size() < kMaxDistinctMarks) boards_.push_back(name_);
  }

  Phase phase_ = Phase::kSignature;
  std::size_t matched_ = 0;
  std::size_t length_ = 0;
  unsigned sum_ = 0;
  std::string name_;
  std::vector<std::string> boards_;
  std::size_t marks_ = 0;
};

}  // namespace armor::image_board
