#!/usr/bin/env python3
# ARMOR-NODE-RADAR - guard on the pulse timer of the pins (main/gpio_manager.cpp). Nothing is compiled: it reads the source and fails when
#   1. the callback of the pulse timer (it runs on the timer task, which serves every other timer of the node) takes the lock that the polling task holds during its sweep;
#   2. the polling task no longer applies the end of a pulse, so an output would stay on;
#   3. a new command no longer forgets an end of pulse that was marked but not applied.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def problems_of(text: str) -> list[str]:
    problems: list[str] = []
    if not text:
        return ["main/gpio_manager.cpp is missing"]
    callback = re.search(r"void\s+pulse_ended\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}", text, re.S)
    if callback is None:
        problems.append("there is no pulse_ended callback")
    elif re.search(r"g_lock|lock_guard|unique_lock|write_output|publish\(", callback.group("body")):
        problems.append("pulse_ended takes the lock, writes the output or publishes: it runs on the timer task and must only mark the end of the pulse")
    if not re.search(r"pulse_over\s*\.\s*exchange\s*\(\s*false\s*\)", text):
        problems.append("the polling task does not apply the end of a pulse (pulse_over.exchange(false))")
    command = re.search(r"Outcome\s+command\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}", text, re.S)
    if command is None or not re.search(r"pulse_over\s*=\s*false", command.group("body")):
        problems.append("a new command does not forget an end of pulse that was marked and not applied")
    return problems


def main() -> int:
    source = ROOT / "main" / "gpio_manager.cpp"
    found = problems_of(source.read_text(encoding="utf-8") if source.exists() else "")
    for line in found:
        print("FAIL", line)
    # the guard itself: a callback that takes the lock must be caught, the fixed one must pass
    bad = "void pulse_ended(void* a) {\n  std::lock_guard<std::mutex> guard(g_lock);\n  x();\n}\n"
    if not any("pulse_ended takes the lock" in p for p in problems_of(bad)):
        print("FAIL the guard does not catch a callback that takes the lock")
        found.append("self-test")
    print("GPIO_TIMER=" + ("FAIL" if found else "PASS"))
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
