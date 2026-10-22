#!/usr/bin/env python3
"""ARMOR-RADAR - the other end of the node's transparent serial bridge to a sensor.

Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.

The node can hand the serial line of one sensor (1 to 3) to an administrator for a few minutes (docs/SENSORS.md, "The serial bridge"): the bytes written here go to the sensor and the bytes the
sensor sends come back, and the node understands none of it. This tool is the PC side of it, with three uses:

    tools/serial_bridge_client.py 192.168.0.181 --sensor 2 --baud 115200 capture --seconds 30 --out ld2410s.hex
        listens to the sensor and writes what it sends, as a hexadecimal dump with the time of each piece (how the first bytes of an LD2410S look after power-up, for instance);
    tools/serial_bridge_client.py 192.168.0.181 --sensor 2 send "FD FC FB FA 04 00 FF 00 01 00 04 03 02 01" --read 64
        writes the bytes (hexadecimal, spaces allowed) and prints what comes back;
    tools/serial_bridge_client.py 192.168.0.181 --sensor 1 tcp --port 5555
        opens a TCP port on THIS computer (127.0.0.1) and passes whatever connects to it to the sensor and back: a program that can speak to a "network serial port" (or a virtual COM port
        made from it) can use the manufacturers' configuration tools through the node.

Nothing here flashes a sensor: the node does not know how (the protocol of the manufacturers' flashing is not documented), and the sensor's supply is always 5 V from the node, so a tool that needs the
sensor to start in its boot loader needs its supply cut by hand. The login is that of an administrator of the node: user and password come from NODE_BRIDGE_USER and NODE_BRIDGE_PASS or are asked
(the password is never printed and never stored). The node's certificate is its own: `--insecure` accepts it (only inside your own network), `--fingerprint` pins its SHA-256.
Only the Python standard library is used.
"""
from __future__ import annotations

import argparse
import getpass
import hashlib
import http.cookiejar
import json
import os
import re
import socket
import ssl
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Callable

MAX_WRITE = 512     # the most the node takes in one exchange
MAX_READ = 1024     # the most it gives back in one exchange
BAUDS = (9600, 19200, 38400, 57600, 115200, 230400, 256000, 460800, 921600)


class BridgeError(RuntimeError):
    pass


def parse_hex(text: str) -> bytes:
    """Bytes from hexadecimal text: spaces, colons and dashes between pairs are allowed, an odd count of digits is not."""
    digits = re.sub(r"[\s:,-]", "", text)
    if not re.fullmatch(r"(?:[0-9A-Fa-f]{2})+", digits):
        raise BridgeError("the bytes must be hexadecimal pairs, like FD FC FB FA")
    return bytes.fromhex(digits)


def dump(piece: bytes, at: float) -> str:
    """One line of a capture: the time since the start and the bytes."""
    return f"{at:8.3f}  {piece.hex(' ').upper()}"


class Bridge:
    """A node's panel with its session cookie, and the bridge session the tool opened."""

    def __init__(self, address: str, *, insecure: bool = False, fingerprint: str = "", timeout: float = 10.0) -> None:
        self.base = address if address.startswith("http") else f"https://{address}"
        self.timeout = timeout
        self.fingerprint = re.sub(r"[^0-9a-f]", "", fingerprint.lower())
        context = ssl.create_default_context()
        if insecure or self.fingerprint:
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE   # the pin below stands in for the chain when one is given
        self.jar = http.cookiejar.CookieJar()
        self.opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(self.jar), urllib.request.HTTPSHandler(context=context))
        self.token = ""
        self._pinned = False

    def _pin(self) -> None:
        if not self.fingerprint or self._pinned or not self.base.startswith("https"):
            return
        host = self.base.split("://", 1)[1].split("/", 1)[0]
        name, _, port = host.partition(":")
        with socket.create_connection((name, int(port or 443)), timeout=self.timeout) as raw:
            context = ssl.create_default_context()
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
            with context.wrap_socket(raw, server_hostname=name) as secure:
                found = hashlib.sha256(secure.getpeercert(binary_form=True) or b"").hexdigest()
        if found != self.fingerprint:
            raise BridgeError(f"the certificate of the node is not the one pinned (its SHA-256 is {found})")
        self._pinned = True

    def call(self, method: str, path: str, body: bytes | dict | None = None, query: str = "", headers: dict[str, str] | None = None) -> tuple[int, bytes]:
        self._pin()
        data = json.dumps(body).encode("utf-8") if isinstance(body, dict) else body
        head = {"X-Requested-With": "armor", **(headers or {})}
        if isinstance(body, dict):
            head["Content-Type"] = "application/json"
        elif data is not None:
            head["Content-Type"] = "application/octet-stream"
        request = urllib.request.Request(f"{self.base}/api/v1/{path}{query}", data=data, method=method, headers=head)
        try:
            with self.opener.open(request, timeout=self.timeout) as response:
                return response.status, response.read()
        except urllib.error.HTTPError as error:
            return error.code, error.read() or b""
        except (urllib.error.URLError, TimeoutError, ConnectionError, OSError) as error:
            raise BridgeError(f"the node does not answer at {self.base}: {error}") from error

    def json_call(self, method: str, path: str, body: dict | None = None) -> dict:
        status, raw = self.call(method, path, body if body is not None else ({} if method == "POST" else None))
        try:
            answer = json.loads(raw or b"{}")
        except ValueError:
            answer = {}
        if status >= 400:
            raise BridgeError(f"{method} {path}: {answer.get('error', status)}")
        return answer

    def login(self, user: str, password: str) -> None:
        self.json_call("POST", "login", {"username": user, "password": password})

    def open(self, sensor: int, baud: int, minutes: int) -> dict:
        opened = self.json_call("POST", "bridge/open", {"sensor": sensor, "baud": baud, "minutes": minutes})
        self.token = str(opened.get("token", ""))
        if not self.token:
            raise BridgeError("the node opened no session")
        return opened

    def exchange(self, outgoing: bytes = b"", read: int = 0, wait_ms: int = 20) -> bytes:
        if len(outgoing) > MAX_WRITE:
            raise BridgeError(f"at most {MAX_WRITE} bytes go in one exchange")
        status, raw = self.call("POST", "bridge/io", outgoing, f"?read={min(read, MAX_READ)}&wait={min(max(wait_ms, 0), 500)}", {"X-Bridge-Token": self.token})
        if status != 200:
            try:
                reason = json.loads(raw or b"{}").get("error", status)
            except ValueError:
                reason = status
            raise BridgeError(f"bridge/io: {reason}")
        return raw

    def close(self) -> None:
        try:
            self.call("POST", "bridge/close", {})
        except BridgeError:
            pass
        self.token = ""


def capture(bridge: Bridge, seconds: float, out: Path | None, sink: Callable[[str], None] = print, pause_s: float = 0.05) -> int:
    """Listens for `seconds` and writes every piece the sensor sends. Returns how many bytes came."""
    started = time.monotonic()
    total = 0
    lines: list[str] = []
    while time.monotonic() - started < seconds:
        piece = bridge.exchange(b"", MAX_READ, 200)
        if piece:
            total += len(piece)
            line = dump(piece, time.monotonic() - started)
            lines.append(line)
            sink(line)
        else:
            time.sleep(pause_s)
    if out is not None:
        out.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    return total


def serve_tcp(bridge: Bridge, port: int, seconds: float, accepted: Callable[[], None] = lambda: None) -> None:
    """Passes the bytes of one TCP client on this computer (127.0.0.1 only) to the sensor and back, until it leaves or the time is over."""
    with socket.socket() as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(("127.0.0.1", port))
        server.listen(1)
        server.settimeout(1.0)
        accepted()
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            try:
                client, _ = server.accept()
            except socket.timeout:
                continue
            with client:
                client.settimeout(0.05)
                while time.monotonic() < deadline:
                    try:
                        outgoing = client.recv(MAX_WRITE)
                        if outgoing == b"":
                            break
                    except socket.timeout:
                        outgoing = b""
                    answer = bridge.exchange(outgoing, MAX_READ, 20)
                    if answer:
                        client.sendall(answer)
            return


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("address", help="the node: an address, or https://address:port")
    parser.add_argument("--sensor", type=int, required=True, choices=(1, 2, 3), help="the sensor (its port on the node)")
    parser.add_argument("--baud", type=int, default=0, choices=(0, *BAUDS), help="the speed of the line; 0: the speed the sensor is set to in the node")
    parser.add_argument("--minutes", type=int, default=0, help="how long the session lasts, 1 to 30 (0: the node's default, 10)")
    parser.add_argument("--insecure", action="store_true", help="accept the node's own certificate without checking it (inside your own network only)")
    parser.add_argument("--fingerprint", default="", help="the SHA-256 of the node's certificate; the connection is made only to a node that shows it")
    commands = parser.add_subparsers(dest="command", required=True)
    cap = commands.add_parser("capture", help="listen to the sensor and write what it sends")
    cap.add_argument("--seconds", type=float, default=30.0)
    cap.add_argument("--out", type=Path, default=None)
    send = commands.add_parser("send", help="write bytes to the sensor and print the answer")
    send.add_argument("bytes", help="hexadecimal bytes, like 'FD FC FB FA 04 00 FF 00 01 00 04 03 02 01'")
    send.add_argument("--read", type=int, default=64)
    send.add_argument("--wait", type=int, default=200, help="milliseconds to wait for the first byte of the answer (0 to 500)")
    tcp = commands.add_parser("tcp", help="open a TCP port on this computer that is the sensor's serial line")
    tcp.add_argument("--port", type=int, default=5555)
    tcp.add_argument("--seconds", type=float, default=600.0)
    args = parser.parse_args(argv)
    user = os.environ.get("NODE_BRIDGE_USER") or input("administrator of the node: ")
    password = os.environ.get("NODE_BRIDGE_PASS") or getpass.getpass("password: ")
    bridge = Bridge(args.address, insecure=args.insecure, fingerprint=args.fingerprint)
    try:
        bridge.login(user, password)
        opened = bridge.open(args.sensor, args.baud, args.minutes)
        print(f"bridge to sensor {opened.get('sensor')} at {opened.get('baud')} baud for {opened.get('minutes')} minutes", file=sys.stderr)
        if args.command == "capture":
            total = capture(bridge, args.seconds, args.out)
            print(f"{total} bytes", file=sys.stderr)
        elif args.command == "send":
            answer = bridge.exchange(parse_hex(args.bytes), args.read, args.wait)
            print(answer.hex(" ").upper() if answer else "(no answer)")
        else:
            serve_tcp(bridge, args.port, args.seconds, lambda: print(f"listening on 127.0.0.1:{args.port}", file=sys.stderr))
    except BridgeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        pass
    finally:
        bridge.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
