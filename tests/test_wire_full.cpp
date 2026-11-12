// ARMOR-NODE-RADAR - the generated readers and writers of the messages (core/armor_wire.hpp) tried with a document that holds EVERY member the contract declares.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
//   test_wire_full <ARMOR-COMMON/conformance/wire_full_cases.tsv>
//
// The valid vectors of the contracts leave out optional members, nested parts and choices that no single vector has, so the code that reads and writes them was never run by test_wire. Every
// line of that file is a message with all of its members (four variants that turn the choices of the enumerations and of the numbers) and the verdict of the Python validator. Here each one
// must be valid for the generated validator, must be read and written back to a document that is valid and stable, and keeps every member it had. The last part feeds the decoder of
// the text patterns text that is not UTF-8, which no JSON document of the files can carry.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#if __has_include("../core/armor_wire.hpp")
#include "../core/armor_wire.hpp"
#else
#include "../main/core/armor_wire.hpp"
#endif

static int failures = 0;
static int checks = 0;
static void expect(bool condition, const std::string& what) {
  ++checks;
  if (!condition && ++failures <= 25) std::printf("FAIL %s\n", what.c_str());
}

static std::size_t members_of(const armor::json::Value& value) {
  std::size_t count = 0;
  if (value.is_object()) {
    count += value.names.size();
    for (const armor::json::Value& child : value.items) count += members_of(child);
  } else if (value.is_array()) {
    for (const armor::json::Value& child : value.items) count += members_of(child);
  }
  return count;
}

static void test_bad_text() {
  using armor::wire::detail::next_code_point;
  std::uint32_t code = 0;
  std::size_t at = 0;
  const std::string one_byte = "a", two = "\xC3\xA9", three = "\xE2\x82\xAC", four = "\xF0\x9F\x98\x80";
  expect(next_code_point(one_byte, at, code) && code == 'a' && at == 1, "a one byte character");
  at = 0;
  expect(next_code_point(two, at, code) && code == 0xE9 && at == 2, "a two byte character");
  at = 0;
  expect(next_code_point(three, at, code) && code == 0x20AC && at == 3, "a three byte character");
  at = 0;
  expect(next_code_point(four, at, code) && code == 0x1F600 && at == 4, "a four byte character");
  const char* broken[] = {"\xFF", "\x80", "\xBF", "\xF8\x88\x80\x80\x80", "\xC3", "\xE2\x82", "\xF0\x9F\x98"};
  for (const char* text : broken) {
    at = 0;
    expect(!next_code_point(text, at, code), std::string("text that is not UTF-8 is refused: ") + std::to_string(static_cast<unsigned char>(text[0])));
  }
  // a pattern read over text that is not UTF-8 never matches, and over a long enough ordinary text it does not run away
  expect(!armor::wire::detail::matches(0, "\xFF"), "a pattern over a broken text");
  expect(!armor::wire::detail::matches(0, std::string("ab\xC3")), "a pattern over a text cut in a character");
  expect(armor::wire::detail::code_points("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80") == 4, "the characters of a text");
  expect(armor::wire::detail::code_points("") == 0, "no characters in no text");
}

int main(int argc, char** argv) {
  if (argc < 2) { std::printf("usage: test_wire_full wire_full_cases.tsv\n"); return 2; }
  std::ifstream in(argv[1], std::ios::binary);
  if (!in) {
    const char* require = std::getenv("ARMOR_REQUIRE_SIBLINGS");
    if (require != nullptr && require[0] != '\0' && require[0] != '0') { std::printf("FAIL the cases with every member are not at %s and ARMOR_REQUIRE_SIBLINGS is set\n", argv[1]); return 1; }
    std::printf("SKIPPED the cases with every member are not at %s\n", argv[1]);
    return 0;
  }
  std::string line;
  std::size_t number = 0, written = 0;
  while (std::getline(in, line)) {
    ++number;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const std::size_t a = line.find('\t'), b = a == std::string::npos ? a : line.find('\t', a + 1);
    if (b == std::string::npos) { expect(false, "line " + std::to_string(number) + " is not a case"); continue; }
    const std::string schema = line.substr(0, a), text = line.substr(b + 1);
    const bool expected = line[a + 1] == '1';
    const std::string where = "case " + std::to_string(number) + " (" + schema + ")";
    armor::json::Value value;
    if (!armor::json::parse(text, value)) { expect(false, where + ": the reader of this node does not read the document"); continue; }
    expect(armor::wire::validate_message(schema, value) == expected, where + ": the validator and the oracle disagree");
    std::string first;
    const bool read = armor::wire::rewrite_message(schema, value, first);
    expect(read == expected, where + ": the reader and the oracle disagree");
    if (!expected || !read) continue;
    armor::json::Value again;
    std::string second;
    const bool stable = armor::json::parse(first, again) && armor::wire::validate_message(schema, again) && armor::wire::rewrite_message(schema, again, second) && second == first;
    expect(stable, where + ": what was written is not valid or not stable");
    if (stable) ++written;
    // nothing is lost on the way: the document written back has as many members as the one read (the writer may order them differently, never drop one)
    expect(members_of(again) == members_of(value), where + ": the writer dropped or added members: " + std::to_string(members_of(value)) + " became " + std::to_string(members_of(again)));
  }
  test_bad_text();
  std::printf("wire with every member: %zu cases, %zu written back, %d checks, %d failures\n", number, written, checks, failures);
  if (number < 30) { std::printf("FAIL too few cases\n"); return 1; }
  return failures == 0 ? 0 : 1;
}
