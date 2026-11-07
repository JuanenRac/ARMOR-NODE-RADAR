// ARMOR-NODE-RADAR - host tests of how much of a log line is kept.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <string>

#if __has_include("../core/log_line.hpp")
#include "../core/log_line.hpp"
#else
#include "../main/core/log_line.hpp"   // RADAR keeps the shared core files under main/
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

using armor::logbuf::kept_length;

static void test_lengths() {
  CHECK(kept_length(0, 224) == 0 && kept_length(-1, 224) == 0 && kept_length(-100, 224) == 0);
  CHECK(kept_length(10, 224) == 10 && kept_length(223, 224) == 223);
  CHECK(kept_length(224, 224) == 223 && kept_length(225, 224) == 223 && kept_length(100000, 224) == 223);   // the terminator keeps one byte
  CHECK(kept_length(5, 0) == 0 && kept_length(5, 1) == 0 && kept_length(5, 2) == 1);
  CHECK(kept_length(armor::logbuf::kSharedLine - 1, armor::logbuf::kSharedLine) == armor::logbuf::kSharedLine - 1);
  CHECK(kept_length(1000, armor::logbuf::kSharedLine) == armor::logbuf::kSharedLine - 1 && kept_length(1000, armor::logbuf::kSpareLine) == armor::logbuf::kSpareLine - 1);
  CHECK(armor::logbuf::kSharedLine > 224 && armor::logbuf::kSpareLine < 224);   // the lines are no longer cut at 223 characters, and the stack buffer is far smaller than before
}

// What the hook does with the numbers: a line formatted with snprintf into a buffer and clipped by the rule is the start of the line, with no overrun.
static void test_against_snprintf() {
  for (const std::size_t capacity : {std::size_t(1), std::size_t(2), std::size_t(8), armor::logbuf::kSpareLine, armor::logbuf::kSharedLine}) {
    for (const std::size_t length : {std::size_t(0), std::size_t(1), std::size_t(7), std::size_t(8), std::size_t(95), std::size_t(96), std::size_t(383), std::size_t(384), std::size_t(500)}) {
      const std::string text(length, 'x');
      std::string buffer(capacity, '\0');
      const int written = std::snprintf(&buffer[0], capacity, "%s", text.c_str());
      const std::size_t kept = kept_length(written, capacity);
      CHECK(kept <= capacity - 1 + (capacity == 0 ? 1 : 0));
      CHECK(kept == std::strlen(buffer.c_str()));   // exactly what is in the buffer: no more, no less
      CHECK(text.compare(0, kept, buffer, 0, kept) == 0);
    }
  }
}

int main() {
  test_lengths();
  test_against_snprintf();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
