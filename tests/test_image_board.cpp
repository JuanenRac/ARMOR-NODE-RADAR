// ARMOR-NODE-RADAR - host tests of the board mark of a firmware image: the mark, the scanner that finds it in an upload that arrives in pieces, and what an upload does about the verdict.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// Nothing here touches hardware: an "image" is a block of bytes built by the test.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#if __has_include("../core/image_board.hpp")
#include "../core/image_board.hpp"
#else
#include "../main/core/image_board.hpp"   // RADAR keeps the shared core files under main/
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

using namespace armor::image_board;

// A pseudo-random block that does not hold the signature by chance (the generator is fixed, and the test checks it).
static std::vector<std::uint8_t> noise(std::size_t length, std::uint32_t seed) {
  std::vector<std::uint8_t> out(length);
  std::uint32_t x = seed;
  for (std::uint8_t& b : out) { x = x * 1664525u + 1013904223u; b = static_cast<std::uint8_t>(x >> 24); }
  return out;
}
template <std::size_t N>
static void append(std::vector<std::uint8_t>& image, const std::array<std::uint8_t, N>& mark) { image.insert(image.end(), mark.begin(), mark.end()); }

static Verdict scan_whole(const std::vector<std::uint8_t>& image, const char* board, std::size_t piece = 0) {
  Scanner scanner;
  if (piece == 0) scanner.feed(image.data(), image.size());
  else for (std::size_t at = 0; at < image.size(); at += piece) scanner.feed(image.data() + at, std::min(piece, image.size() - at));
  return scanner.verdict(board);
}

static void test_the_mark() {
  const auto eth = make_mark("s3-eth");
  CHECK(eth.size() == kSignatureBytes + 1 + 6 + 2);
  for (std::size_t i = 0; i < kSignatureBytes; ++i) CHECK(eth[i] == signature_byte(i));
  CHECK(eth[kSignatureBytes] == 6 && eth[kSignatureBytes + 1] == 's' && eth[kSignatureBytes + 7] == 0);
  CHECK(make_mark("s3-wifi").size() == eth.size() + 1 && make_mark("s3-wifi")[kSignatureBytes + 1] == 's');
  // the mark of this build is one of the two boards'
#if defined(ARMOR_BOARD_S3_ETH)
  CHECK(kOwnMark == make_mark("s3-eth"));
#else
  CHECK(kOwnMark == make_mark("s3-wifi"));
#endif
  CHECK(keep() == kOwnMark[kSignatureBytes]);
  // no byte of the signature repeats, which is what lets the scanner restart on one byte
  CHECK(signature_bytes_differ());
  CHECK(board_char_ok('a') && board_char_ok('9') && board_char_ok('-') && !board_char_ok('A') && !board_char_ok(' ') && !board_char_ok(0));
}

static void test_scanner_verdicts() {
  std::vector<std::uint8_t> image = noise(5000, 1);
  CHECK(scan_whole(image, "s3-eth") == Verdict::kNoMark);   // an image made before the marks
  append(image, make_mark("s3-eth"));
  const std::vector<std::uint8_t> tail = noise(7000, 2);
  image.insert(image.end(), tail.begin(), tail.end());
  CHECK(scan_whole(image, "s3-eth") == Verdict::kMatch);
  CHECK(scan_whole(image, "s3-wifi") == Verdict::kOtherBoard);
  // the same answer whatever the size of the pieces the upload arrives in, down to one byte at a time (the mark is cut at every possible place)
  for (const std::size_t piece : {1u, 2u, 3u, 7u, 8u, 9u, 16u, 255u, 2048u}) {
    CHECK(scan_whole(image, "s3-eth", piece) == Verdict::kMatch);
    CHECK(scan_whole(image, "s3-wifi", piece) == Verdict::kOtherBoard);
  }
  // a mark at the very start and at the very end of the image
  std::vector<std::uint8_t> first;
  append(first, make_mark("s3-wifi"));
  first.insert(first.end(), tail.begin(), tail.end());
  CHECK(scan_whole(first, "s3-wifi") == Verdict::kMatch && scan_whole(first, "s3-wifi", 5) == Verdict::kMatch);
  std::vector<std::uint8_t> last = noise(100, 3);
  append(last, make_mark("s3-eth"));
  CHECK(scan_whole(last, "s3-eth") == Verdict::kMatch && scan_whole(last, "s3-eth", 1) == Verdict::kMatch);
}

static void test_scanner_ignores_what_is_not_a_mark() {
  const auto eth = make_mark("s3-eth");
  // the bare signature, as the code that looks for it may hold it: followed by anything else it is not a mark
  std::vector<std::uint8_t> decoy = noise(300, 4);
  for (std::size_t i = 0; i < kSignatureBytes; ++i) decoy.push_back(signature_byte(i));
  for (const std::uint8_t b : {std::uint8_t{200}, std::uint8_t{1}, std::uint8_t{2}}) decoy.push_back(b);   // a length that is too long
  decoy.insert(decoy.end(), 100, 0xFF);
  CHECK(scan_whole(decoy, "s3-eth") == Verdict::kNoMark);
  // the decoy and then the real mark: the real one counts
  append(decoy, eth);
  CHECK(scan_whole(decoy, "s3-eth") == Verdict::kMatch && scan_whole(decoy, "s3-wifi") == Verdict::kOtherBoard);
  // a mark whose check byte is wrong, whose name is not a board's, or whose zero is missing is not a mark
  auto broken = std::vector<std::uint8_t>(eth.begin(), eth.end());
  broken.back() ^= 0x01;
  CHECK(scan_whole(broken, "s3-eth") == Verdict::kNoMark);
  broken = std::vector<std::uint8_t>(eth.begin(), eth.end());
  broken[kSignatureBytes + 2] = 'S';   // capital letters are not in a board's name
  CHECK(scan_whole(broken, "s3-eth") == Verdict::kNoMark);
  broken = std::vector<std::uint8_t>(eth.begin(), eth.end());
  broken[kSignatureBytes + 1 + 6] = 7;   // the zero after the name
  CHECK(scan_whole(broken, "s3-eth") == Verdict::kNoMark);
  broken = std::vector<std::uint8_t>(eth.begin(), eth.end());
  broken[kSignatureBytes] = 0;   // a name of no characters
  CHECK(scan_whole(broken, "s3-eth") == Verdict::kNoMark);
  broken = std::vector<std::uint8_t>(eth.begin(), eth.end());
  broken.resize(broken.size() - 1);   // an image that ends inside the mark
  CHECK(scan_whole(broken, "s3-eth") == Verdict::kNoMark);
  // a signature that begins inside the bytes that broke another mark is still found
  std::vector<std::uint8_t> restart(eth.begin(), eth.begin() + 5);
  append(restart, eth);
  CHECK(scan_whole(restart, "s3-eth") == Verdict::kMatch);
  // a broken mark right before a good one: the good one counts
  std::vector<std::uint8_t> both(eth.begin(), eth.end());
  both.back() ^= 0x10;
  append(both, make_mark("s3-wifi"));
  CHECK(scan_whole(both, "s3-wifi") == Verdict::kMatch && scan_whole(both, "s3-eth") == Verdict::kOtherBoard);
}

static void test_two_boards_in_one_image() {
  // an image that carries the marks of both boards is the other board's as far as either board is concerned: it is never accepted on the strength of one of them
  std::vector<std::uint8_t> image = noise(64, 5);
  append(image, make_mark("s3-eth"));
  image.insert(image.end(), 10, 0);
  append(image, make_mark("s3-wifi"));
  Scanner scanner;
  scanner.feed(image.data(), image.size());
  CHECK(scanner.marks_seen() == 2 && scanner.boards().size() == 2);
  CHECK(scanner.verdict("s3-eth") == Verdict::kOtherBoard && scanner.verdict("s3-wifi") == Verdict::kOtherBoard);
  // the same mark twice is one board; many different ones are remembered up to a limit and still never a match
  Scanner twice;
  std::vector<std::uint8_t> same = noise(8, 6);
  append(same, make_mark("s3-eth"));
  append(same, make_mark("s3-eth"));
  twice.feed(same.data(), same.size());
  CHECK(twice.marks_seen() == 2 && twice.boards().size() == 1 && twice.verdict("s3-eth") == Verdict::kMatch);
  Scanner many;
  for (const char* name : {"a", "b", "c", "d", "e", "f"}) {
    std::vector<std::uint8_t> one(kSignatureBytes);
    for (std::size_t i = 0; i < kSignatureBytes; ++i) one[i] = signature_byte(i);
    one.push_back(1); one.push_back(static_cast<std::uint8_t>(name[0])); one.push_back(0);
    one.push_back(static_cast<std::uint8_t>((static_cast<unsigned char>(name[0]) ^ 0x5Cu) & 0xFFu));
    many.feed(one.data(), one.size());
  }
  CHECK(many.marks_seen() == 6 && many.boards().size() == kMaxDistinctMarks && many.verdict("a") == Verdict::kOtherBoard);
  // a string_view feeds the same
  Scanner view;
  const auto wifi = make_mark("s3-wifi");
  view.feed(std::string_view(reinterpret_cast<const char*>(wifi.data()), wifi.size()));
  CHECK(view.verdict("s3-wifi") == Verdict::kMatch);
}

static void test_policy() {
  Policy strict;   // the defaults
  CHECK(strict.check_board && strict.require_mark);
  CHECK(decide(Verdict::kMatch, strict) == Decision::kAccept);
  CHECK(decide(Verdict::kOtherBoard, strict) == Decision::kWrongBoard);
  CHECK(decide(Verdict::kNoMark, strict) == Decision::kNoBoardMark);
  Policy allow_old;
  allow_old.require_mark = false;
  CHECK(decide(Verdict::kNoMark, allow_old) == Decision::kAccept);
  CHECK(decide(Verdict::kOtherBoard, allow_old) == Decision::kWrongBoard);   // another board's image is refused whatever the second switch says
  Policy off;
  off.check_board = false;
  CHECK(decide(Verdict::kOtherBoard, off) == Decision::kAccept && decide(Verdict::kNoMark, off) == Decision::kAccept && decide(Verdict::kMatch, off) == Decision::kAccept);
  CHECK(std::string(decision_code(Decision::kWrongBoard)) == "wrong_board" && std::string(decision_code(Decision::kNoBoardMark)) == "no_board_mark" && std::string(decision_code(Decision::kAccept)).empty());
}

int main() {
  test_the_mark();
  test_scanner_verdicts();
  test_scanner_ignores_what_is_not_a_mark();
  test_two_boards_in_one_image();
  test_policy();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
