// ARMOR-NODE-RADAR - how much of a formatted log line is kept in the log of the panel. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The hook of the log formats every line of every task into a buffer. A buffer on the stack of the task that logs cost every task 224 bytes (the small ones, of 3 KiB, are the tight ones) and cut
// the lines at 223 characters; the buffer is now one for all the tasks, under a lock that is only tried (a task never waits to log), with a smaller one on the stack as the way out when the lock is
// taken. The rule of what is kept of a line lives here.
#pragma once
#include <cstddef>

namespace armor::logbuf {

constexpr std::size_t kSharedLine = 384;   // the buffer of everyone, in static memory
constexpr std::size_t kSpareLine = 96;     // the one on the stack of the task that finds the first one taken

// How many characters of a line that vsnprintf says it wanted to write (`written`, negative on an error) fit in a buffer of `capacity` bytes (the last one is the terminator).
constexpr std::size_t kept_length(int written, std::size_t capacity) {
  if (written <= 0 || capacity == 0) return 0;
  const std::size_t wanted = static_cast<std::size_t>(written);
  return wanted < capacity ? wanted : capacity - 1;
}

}  // namespace armor::logbuf
