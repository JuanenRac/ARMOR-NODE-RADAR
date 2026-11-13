#!/usr/bin/env python3
# ARMOR-NODE-RADAR - guard on the firmware upload of the panel's server. Nothing is compiled or flashed: main/web_server.cpp is read, and this fails when
#   1. the upload does not feed every received chunk to the board-mark scanner, or does not decide on the mark AFTER the image was closed and BEFORE it becomes the boot slot;
#   2. an image made for another board is not answered with 422 "wrong_board" before esp_ota_set_boot_partition(target);
#   3. the switch to the other slot does not look at the mark of that slot before esp_ota_set_boot_partition(other);
#   4. a failed or interrupted write does not abort the update handle.
# The decision itself (what is accepted, what is refused) is tested in test_image_board.cpp; this keeps the server from skipping it. A damaged copy of the source is also tried, so the guard is shown to fail.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def read(relative: str) -> str:
    path = ROOT / relative
    return path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""


def problems_of(source: str) -> list[str]:
    if not source:
        return ["main/web_server.cpp is missing"]
    problems: list[str] = []
    begin = source.find("esp_ota_begin(target")
    if begin < 0:
        return ["the upload no longer opens the update with esp_ota_begin(target"]
    boot = source.find("esp_ota_set_boot_partition(target", begin)
    if boot < 0:
        return ["the upload no longer sets the new slot with esp_ota_set_boot_partition(target"]
    upload = source[begin:boot]
    feed = upload.find("board_scanner.feed(")
    end = upload.find("esp_ota_end(")
    decide = upload.find("image_board::decide(board_scanner.verdict(")
    refuse = upload.find('send_error(r, 422, "wrong_board")')
    if feed < 0:
        problems.append("the upload does not feed the chunks to board_scanner, so the mark of the image is never read")
    if end < 0:
        problems.append("the upload does not close the image with esp_ota_end before it is decided")
    if decide < 0:
        problems.append("the upload does not decide on board_scanner.verdict(...) with image_board::decide")
    if refuse < 0:
        problems.append('the upload does not answer 422 "wrong_board" for an image made for another board')
    if min(feed, end, decide, refuse) >= 0 and not feed < end < decide < refuse:
        problems.append("the order of the upload must be: scan the chunks, close the image, decide on the mark, refuse a wrong board, and only then set the boot slot")
    if "board_decision != image_board::Decision::kAccept" not in upload:
        problems.append("the upload refuses only some of the decisions: anything other than kAccept must stop it before the boot slot is set")
    if upload.count("esp_ota_abort(handle)") < 2:
        problems.append("a failed write or an interrupted upload must call esp_ota_abort(handle)")
    switch = source.find("esp_ota_set_boot_partition(other")
    if switch < 0:
        problems.append("the switch to the other slot no longer uses esp_ota_set_boot_partition(other")
    else:
        head = source.rfind("esp_err_t post_ota_switch(", 0, switch)
        part = source[head:switch] if head >= 0 else ""
        if "image_board::decide(slot_board_verdict(other)" not in part:
            problems.append("the switch to the other slot does not decide on the mark of that slot before setting it")
        if 'send_error(r, 422, "wrong_board")' not in part:
            problems.append('the switch to the other slot does not answer 422 "wrong_board" for a slot made for another board')
    return problems


def damaged(source: str) -> list[tuple[str, str]]:
    """Copies of the source with one guard taken out; every one of them has to be refused."""
    return [
        ("scanner not fed", source.replace("board_scanner.feed(", "(void)(", 1)),
        ("wrong board not refused", source.replace('send_error(r, 422, "wrong_board")', 'ESP_OK', 1)),
        ("decision after the boot slot", source.replace("esp_ota_set_boot_partition(target", "esp_ota_set_boot_partition(target", 1).replace(
            "image_board::decide(board_scanner.verdict(", "image_board_skipped(board_scanner.verdict(", 1)),
        ("other slot not looked at", source.replace("image_board::decide(slot_board_verdict(other)", "image_board_skipped(slot_board_verdict(other)", 1)),
    ]


def main() -> int:
    source = read("main/web_server.cpp")
    problems = problems_of(source)
    for name, text in damaged(source):
        if text == source:
            problems.append(f"self-test '{name}': the pattern to damage was not found, the guard cannot prove itself")
        elif not problems_of(text):
            problems.append(f"self-test '{name}': a damaged server was accepted")
    if problems:
        print("OTA_BOARD=FAIL")
        for problem in problems:
            print("  " + problem)
        return 1
    print("OTA_BOARD=PASS the upload reads the mark, refuses a wrong board before the boot slot, and the switch looks at the other slot")
    return 0


if __name__ == "__main__":
    sys.exit(main())
