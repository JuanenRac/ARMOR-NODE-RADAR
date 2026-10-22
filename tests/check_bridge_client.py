#!/usr/bin/env python3
"""Checks tools/serial_bridge_client.py against a node made up for the test: the login, the session, the token on every exchange, the limits of an exchange, the capture, the TCP door of this computer
and the hexadecimal reader. Nothing leaves this computer.

Usage:  python tests/check_bridge_client.py
"""
from __future__ import annotations

import json
import socket
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import serial_bridge_client as client  # noqa: E402

failures: list[str] = []


def check(condition: bool, what: str) -> None:
    if not condition:
        failures.append(what)


class FakeNode(BaseHTTPRequestHandler):
    state: dict = {}

    def log_message(self, *args) -> None:   # noqa: D401 - silence
        pass

    def _send(self, code: int, body: bytes = b"{}", kind: str = "application/json", cookie: str = "") -> None:
        self.send_response(code)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(body)))
        if cookie:
            self.send_header("Set-Cookie", cookie)
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self) -> None:   # noqa: N802
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length) if length else b""
        state = self.state
        state.setdefault("calls", []).append((self.path, self.headers.get("X-Requested-With"), self.headers.get("X-Bridge-Token")))
        if self.path == "/api/v1/login":
            who = json.loads(body or b"{}")
            if who.get("password") != "right":
                return self._send(401, b'{"error":"bad_credentials"}')
            return self._send(200, b'{"ok":true}', cookie="session=abc; Path=/")
        if "session=abc" not in (self.headers.get("Cookie") or ""):
            return self._send(401, b'{"error":"login_required"}')
        if self.path == "/api/v1/bridge/open":
            wanted = json.loads(body or b"{}")
            state["opened"] = wanted
            return self._send(200, json.dumps({"ok": True, "token": "T0KEN", "sensor": wanted["sensor"], "baud": wanted["baud"] or 115200, "minutes": wanted["minutes"] or 10}).encode())
        if self.path == "/api/v1/bridge/close":
            state["closed"] = True
            return self._send(200, b'{"ok":true}')
        if self.path.startswith("/api/v1/bridge/io"):
            if self.headers.get("X-Bridge-Token") != "T0KEN":
                return self._send(403, b'{"error":"bridge_bad_token"}')
            if len(body) > client.MAX_WRITE:
                return self._send(413, b'{"error":"bridge_too_big"}')
            state.setdefault("written", b"")
            state["written"] += body
            queue = state.setdefault("to_pc", [b"\xfd\xfc\xfb\xfa", b"\x04\x03\x02\x01"])
            answer = b"".join(queue) if body else (queue.pop(0) if queue else b"")
            if body:
                queue.clear()
            return self._send(200, answer, "application/octet-stream")
        return self._send(404)


def with_node(run) -> dict:
    FakeNode.state = {}
    server = ThreadingHTTPServer(("127.0.0.1", 0), FakeNode)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        run(f"http://127.0.0.1:{server.server_address[1]}")
    finally:
        server.shutdown()
        server.server_close()
    return FakeNode.state


def test_hex() -> None:
    check(client.parse_hex("FD FC FB FA") == b"\xfd\xfc\xfb\xfa", "spaces between pairs")
    check(client.parse_hex("fd:fc-fb,fa") == b"\xfd\xfc\xfb\xfa", "colons, dashes and commas")
    for bad in ("", "F", "ZZ", "FD FC F"):
        try:
            client.parse_hex(bad)
            check(False, f"'{bad}' is not hexadecimal pairs")
        except client.BridgeError:
            pass
    check(client.dump(b"\x01\xab", 1.5).endswith("01 AB"), "a dump line has the bytes in capitals")


def test_session() -> None:
    results: dict = {}

    def run(base: str) -> None:
        bridge = client.Bridge(base)
        try:
            bridge.login("ana", "wrong")
            check(False, "a wrong password is refused")
        except client.BridgeError as error:
            check("bad_credentials" in str(error), "the reason of the refusal is told")
        bridge.login("ana", "right")
        opened = bridge.open(2, 115200, 5)
        check(opened["sensor"] == 2 and bridge.token == "T0KEN", "the session is opened and the token kept")
        results["answer"] = bridge.exchange(b"\xfd\xfc", 16, 20)
        try:
            bridge.exchange(b"x" * (client.MAX_WRITE + 1))
            check(False, "an exchange over the limit is refused here")
        except client.BridgeError:
            pass
        bridge.token = "wrong"
        try:
            bridge.exchange(b"\x00", 1)
            check(False, "a wrong token is refused by the node")
        except client.BridgeError as error:
            check("bridge_bad_token" in str(error), "the node's reason is shown")
        bridge.token = "T0KEN"
        bridge.close()
        check(bridge.token == "", "the token is forgotten when the session ends")

    state = with_node(run)
    check(results.get("answer") == b"\xfd\xfc\xfb\xfa\x04\x03\x02\x01", "what the sensor sent comes back")
    check(state.get("written") == b"\xfd\xfc", "what was written reaches the node")
    check(state.get("opened") == {"sensor": 2, "baud": 115200, "minutes": 5}, "the session asks for what the tool was told")
    check(state.get("closed") is True, "the session is closed")
    check(all(call[1] == "armor" for call in state["calls"]), "every call carries the header the panel asks for")
    check(all(call[2] in (None, "T0KEN", "wrong") for call in state["calls"]), "the token travels only in its header")


def test_capture(tmp: Path) -> None:
    out = tmp / "capture.hex"
    seen: list[str] = []

    def run(base: str) -> None:
        bridge = client.Bridge(base)
        bridge.login("ana", "right")
        bridge.open(1, 0, 0)
        total = client.capture(bridge, 0.4, out, seen.append, pause_s=0.01)
        check(total == 8, "a capture counts the bytes")
        bridge.close()

    with_node(run)
    check(len(seen) == 2 and seen[0].endswith("FD FC FB FA"), "each piece is a line")
    check(out.read_text(encoding="utf-8").count("\n") == 2, "the file has a line for each piece")


def test_tcp() -> None:
    port = socket.socket()
    port.bind(("127.0.0.1", 0))
    free = port.getsockname()[1]
    port.close()
    received: list[bytes] = []

    def run(base: str) -> None:
        bridge = client.Bridge(base)
        bridge.login("ana", "right")
        bridge.open(1, 115200, 5)
        ready = threading.Event()
        worker = threading.Thread(target=client.serve_tcp, args=(bridge, free, 3.0, ready.set), daemon=True)
        worker.start()
        ready.wait(2)
        time.sleep(0.1)
        with socket.create_connection(("127.0.0.1", free), timeout=2) as door:
            door.sendall(b"\x01\x02")
            door.settimeout(2)
            received.append(door.recv(64))
        worker.join(5)
        bridge.close()

    state = with_node(run)
    check(state.get("written") == b"\x01\x02", "what a program writes to the port goes to the sensor")
    check(received and received[0] == b"\xfd\xfc\xfb\xfa\x04\x03\x02\x01", "what the sensor sends comes back to the program")


def main() -> int:
    import tempfile

    test_hex()
    test_session()
    with tempfile.TemporaryDirectory() as tmp:
        test_capture(Path(tmp))
    test_tcp()
    if failures:
        for what in failures:
            print(f"BRIDGE_CLIENT=FAIL {what}")
        return 1
    print("BRIDGE_CLIENT=PASS the login, the session, the token, the limits, the capture and the TCP door")
    return 0


if __name__ == "__main__":
    sys.exit(main())
