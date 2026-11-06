// ARMOR-NODE-RADAR - the generated readers and writers of the messages (core/armor_wire.hpp) and this node's own parsers, against the oracle of ARMOR-COMMON.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
//   test_wire <ARMOR-COMMON/conformance/wire_cases.tsv>
//
// Every case of that file has the verdict of the Python validator (the valid vectors of the contracts and seeded mutations of them).
//   1. The generated validator, reader and writer of this copy of the header agree with the verdict, on this compiler and with the flags of this project.
//   2. A parser this node has written by hand for a message it receives (the hooks below, and those of tests/wire_hooks.hpp when the node has one) must not accept a document the
//      contract refuses. What it refuses although the contract accepts it is counted and printed: a node may be stricter (the rules between two fields, the node's own name), never looser.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#if __has_include("../core/armor_wire.hpp")
#include "../core/armor_wire.hpp"
#include "../core/control_gate.hpp"
#else
#include "../main/core/armor_wire.hpp"
#include "../main/core/control_gate.hpp"
#endif

struct Hook {
  const char* schema;
  const char* what;
  std::function<bool(const std::string& text)> accepts;
  const char* may_omit = nullptr;   // a boolean member the contract requires and this parser, on purpose, does not: a document that breaks the contract only by leaving it out is tolerated (and counted)
};

#if __has_include("wire_hooks.hpp")
#include "wire_hooks.hpp"
#else
static std::vector<Hook> node_hooks() { return {}; }
#endif

static int failures = 0;
static void fail(const std::string& what) {
  if (++failures <= 25) std::printf("FAIL %s\n", what.c_str());
}

int main(int argc, char** argv) {
  if (argc < 2) { std::printf("usage: test_wire wire_cases.tsv\n"); return 2; }
  std::ifstream in(argv[1], std::ios::binary);
  if (!in) {
    const char* require = std::getenv("ARMOR_REQUIRE_SIBLINGS");
    if (require != nullptr && require[0] != '\0' && require[0] != '0') { std::printf("FAIL the seeded cases are not at %s and ARMOR_REQUIRE_SIBLINGS is set\n", argv[1]); return 1; }
    std::printf("SKIPPED the seeded cases are not at %s\n", argv[1]);
    return 0;
  }
  std::vector<Hook> hooks = node_hooks();
  hooks.push_back({"node_control_command", "control_gate::parse_command", [](const std::string& text) {
    armor::control::Command parsed;
    return armor::control::parse_command(text, parsed);
  }});

  std::string line;
  std::size_t number = 0, accepted = 0, rewritten = 0;
  std::map<std::string, std::pair<std::size_t, std::size_t>> stricter;   // hook -> (valid cases it refused, valid cases)
  std::map<std::string, std::size_t> tolerated;                          // hook -> documents it accepted that lack only the member it may omit
  while (std::getline(in, line)) {
    ++number;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const std::size_t a = line.find('\t'), b = a == std::string::npos ? a : line.find('\t', a + 1);
    if (b == std::string::npos) { fail("line " + std::to_string(number) + " is not a case"); continue; }
    const std::string schema = line.substr(0, a), text = line.substr(b + 1);
    const bool expected = line[a + 1] == '1';
    const std::string where = "case " + std::to_string(number) + " (" + schema + ")";
    armor::json::Value value;
    if (!armor::json::parse(text, value)) { fail(where + ": the reader of this node does not read the document at all"); continue; }
    const bool got = armor::wire::validate_message(schema, value);
    if (got != expected) fail(where + ": the oracle says " + (expected ? "valid" : "invalid") + ", the generated validator says " + (got ? "valid" : "invalid") + ": " + text.substr(0, 160));
    if (got) ++accepted;
    std::string first;
    const bool read = armor::wire::rewrite_message(schema, value, first);
    if (read != expected) fail(where + ": the generated reader and the oracle disagree");
    if (expected && read) {
      armor::json::Value again;
      std::string second;
      if (!armor::json::parse(first, again) || !armor::wire::validate_message(schema, again) || !armor::wire::rewrite_message(schema, again, second) || second != first) fail(where + ": what was written is not valid or not stable");
      else ++rewritten;
    }
    for (const Hook& hook : hooks) {
      if (schema != hook.schema) continue;
      const bool hand = hook.accepts(text);
      const std::string key = std::string(hook.schema) + " by " + hook.what;
      if (hand && !expected) {
        bool excused = false;
        if (hook.may_omit != nullptr && value.is_object() && value.get(hook.may_omit) == nullptr) {
          armor::json::Value patched = value, flag;   // the same document with the member put in: if it is valid then, the member was the only thing wrong
          flag.type = armor::json::Type::kBool;
          patched.names.push_back(hook.may_omit);
          patched.items.push_back(flag);
          excused = armor::wire::validate_message(schema, patched);
        }
        if (excused) ++tolerated[key];
        else fail(where + ": " + hook.what + " accepts what the contract refuses: " + text.substr(0, 160));
      }
      if (expected) {
        ++stricter[key].second;
        if (!hand) ++stricter[key].first;
      }
    }
  }
  for (const auto& entry : stricter) std::printf("%s: refuses %zu of %zu valid cases (stricter than the contract: allowed)\n", entry.first.c_str(), entry.second.first, entry.second.second);
  for (const auto& entry : tolerated) std::printf("%s: accepts %zu documents that only leave out a member the contract requires (tolerated on purpose)\n", entry.first.c_str(), entry.second);
  std::printf("wire oracle: %zu cases, %zu valid, %zu written back, %d failures\n", number, accepted, rewritten, failures);
  if (number < 100) { std::printf("FAIL too few cases\n"); return 1; }
  return failures == 0 ? 0 : 1;
}
