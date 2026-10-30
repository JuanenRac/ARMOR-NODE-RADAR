#!/usr/bin/env python3
"""A real Mosquitto broker and a stand-in for the node's MQTT client (esp-mqtt).

Usage:  emit_samples | python tests/check_mosquitto.py
        python tests/check_mosquitto.py --binary build/host/emit_samples

The firmware cannot run on a computer, but its MQTT client is a known quantity: esp-mqtt connects with the node's id as client id, a user and a password, a keep-alive, a
retained last will on the node's health topic at QoS 1, a retained "online" message after the connection, a subscription to its own order topic at QoS 1, and publishes at
QoS 1. This script starts a real broker on a free port of the loopback address with the account rules of a node (the same shape as ARMOR-DEVOPS's
`scripts/mqtt_identity.sh`), plays that client with sockets and a small MQTT 3.1.1 codec, and checks what the server would see:

  * the retained "online" message reaches a server that joins later;
  * every message the node's serialiser prints reaches the server byte for byte on the topic of its kind;
  * an order published to the node's order topic reaches the node, and is acknowledged;
  * the node cannot publish on the topic of another node (the broker drops it);
  * when the node's connection dies without a goodbye, the server receives the retained last will ("online": false).

Mosquitto is needed: where it is not installed the check says so and passes, unless ARMOR_REQUIRE_MOSQUITTO is set (the CI of the node repositories installs it and sets it, so a
missing broker there is a failure and not a green run that checked nothing).
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

WINDOWS_BROKER = Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "mosquitto"

# the user a node of each kind connects as (ARMOR-DEVOPS scripts/mqtt_identity.sh), by the project it belongs to
IDENTITIES = {
    "ARMOR-NODE-RADAR": "field-node", "ARMOR-NODE-SOLAR": "solar-node", "ARMOR-NODE-ELECTRICAL": "electrical-node",
    "ARMOR-NODE-ALARM": "alarm-node", "ARMOR-NODE-HMI": "hmi-node",
}
SOLAR_KINDS = {"inverter", "battery", "solar", "solar_inverter", "solar_battery"}


def topic_of(kind: str, node: str) -> str | None:
    """The topic a message of this kind goes to, or None for a line that is not an MQTT message of a node."""
    fixed = {
        "telemetry": f"armor/node/{node}/telemetry", "radar_event": f"armor/radar/{node}/events", "node_health": f"armor/health/{node}", "health": f"armor/health/{node}",
        "electrical": f"armor/electrical/{node}/state", "electrical_result": f"armor/electrical/{node}/result",
        "alarm": f"armor/alarm/{node}/state", "alarm_result": f"armor/alarm/{node}/result",
    }
    if kind in fixed:
        return fixed[kind]
    if kind in SOLAR_KINDS:
        return f"armor/solar/{node}/{kind}"
    return None


# the leaves of the topics a node only READS (the server writes them): its orders and what the server tells it
TO_NODE_LEAVES = {"command", "policy", "zigbee", "armed"}


def classify(token: str, payload: dict, fallback: str) -> tuple[str, str, bool] | None:
    """(topic, node, written_by_the_node) for one printed line, or None when the line is not an MQTT message. A line starts with the topic itself (the solar, electrical and alarm nodes) or with the
    name of the kind of message (the radar node)."""
    if token.startswith("armor/"):
        parts = token.split("/")
        if len(parts) < 3:
            return None
        return token, parts[2], parts[-1] not in TO_NODE_LEAVES
    node = payload.get("node_id") if isinstance(payload.get("node_id"), str) else fallback
    topic = topic_of(token, node)
    return (topic, node, True) if topic else None


# ---- a small MQTT 3.1.1 codec ----------------------------------------------------------------------------------------------------------------

def _text(value: str) -> bytes:
    raw = value.encode("utf-8")
    return struct.pack(">H", len(raw)) + raw


def _length(size: int) -> bytes:
    out = bytearray()
    while True:
        byte, size = size % 128, size // 128
        out.append(byte | (0x80 if size else 0))
        if not size:
            return bytes(out)


class Client:
    """What esp-mqtt does on the wire, and nothing else."""

    def __init__(self, port: int, client_id: str, user: str, password: str, keepalive: int = 20, will: tuple[str, bytes] | None = None) -> None:
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=5)
        self.sock.settimeout(5)
        self.packet_id = 0
        self.inbox: list[tuple[str, bytes, bool]] = []   # (topic, payload, retained)
        flags = 0x02 | 0x80 | 0x40   # clean session, user name, password
        body = _text("MQTT") + bytes([4])
        payload = _text(client_id)
        if will:
            flags |= 0x04 | (1 << 3) | 0x20   # will, QoS 1, retained
            payload += _text(will[0]) + struct.pack(">H", len(will[1])) + will[1]
        payload += _text(user) + _text(password)
        body += bytes([flags]) + struct.pack(">H", keepalive) + payload
        self._send(0x10, body)
        kind, data = self._read()
        if kind != 0x20 or len(data) != 2 or data[1] != 0:
            raise AssertionError(f"the broker refused {client_id} (CONNACK {data!r})")

    def _send(self, first: int, body: bytes) -> None:
        self.sock.sendall(bytes([first]) + _length(len(body)) + body)

    def _read(self, timeout: float = 5) -> tuple[int, bytes]:
        self.sock.settimeout(timeout)
        first = self.sock.recv(1)
        if not first:
            raise ConnectionError("closed")
        size, shift = 0, 0
        while True:
            byte = self.sock.recv(1)[0]
            size |= (byte & 0x7F) << shift
            shift += 7
            if not byte & 0x80:
                break
        data = b""
        while len(data) < size:
            chunk = self.sock.recv(size - len(data))
            if not chunk:
                raise ConnectionError("closed")
            data += chunk
        return first[0], data

    def _next_id(self) -> int:
        self.packet_id = self.packet_id % 65535 + 1
        return self.packet_id

    def _take(self, first: int, data: bytes) -> None:
        if first >> 4 == 3:   # PUBLISH from the broker
            qos, retained = (first >> 1) & 3, bool(first & 1)
            size = struct.unpack(">H", data[:2])[0]
            topic, rest = data[2:2 + size].decode("utf-8"), data[2 + size:]
            if qos:
                self._send(0x40, rest[:2])   # PUBACK
                rest = rest[2:]
            self.inbox.append((topic, rest, retained))

    def pump(self, seconds: float) -> None:
        """Read whatever the broker sends for this long."""
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            try:
                first, data = self._read(max(0.05, end - time.monotonic()))
            except (socket.timeout, TimeoutError):
                return
            self._take(first, data)

    def subscribe(self, topic_filter: str) -> None:
        pid = self._next_id()
        self._send(0x82, struct.pack(">H", pid) + _text(topic_filter) + bytes([1]))
        while True:
            first, data = self._read()
            if first >> 4 == 9:
                if data[2:3] == b"\x80":
                    raise AssertionError(f"the broker refused the subscription to {topic_filter}")
                return
            self._take(first, data)

    def publish(self, topic: str, payload: bytes, retain: bool = False) -> None:
        pid = self._next_id()
        self._send(0x32 | (1 if retain else 0), _text(topic) + struct.pack(">H", pid) + payload)
        while True:
            first, data = self._read()
            if first >> 4 == 4 and data[:2] == struct.pack(">H", pid):
                return
            self._take(first, data)

    def received(self, topic: str, wait: float = 3) -> list[bytes]:
        """The payloads that arrived on this topic, waiting up to `wait` seconds for the first."""
        end = time.monotonic() + wait
        while time.monotonic() < end and not any(item[0] == topic for item in self.inbox):
            self.pump(0.2)
        self.pump(0.2)
        return [item[1] for item in self.inbox if item[0] == topic]

    def drop(self) -> None:
        """The connection dies with no goodbye: what a node that lost power or its network does."""
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
        self.sock.close()

    def close(self) -> None:
        try:
            self._send(0xE0, b"")
        except OSError:
            pass
        self.sock.close()


# ---- the broker ------------------------------------------------------------------------------------------------------------------------------

def find(program: str) -> str | None:
    return shutil.which(program) or next((str(path) for path in (WINDOWS_BROKER / f"{program}.exe", Path("/usr/sbin") / program) if path.is_file()), None)


def free_port() -> int:
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def start_broker(folder: Path, accounts: dict[str, str], acl: str) -> tuple[subprocess.Popen, int]:
    broker, passwd = find("mosquitto"), find("mosquitto_passwd")
    assert broker and passwd
    port = free_port()
    password_file = folder / "passwords"
    first = True
    for user, password in accounts.items():
        subprocess.run([passwd, "-b"] + (["-c"] if first else []) + [str(password_file), user, password], check=True, capture_output=True)
        first = False
    (folder / "acl").write_text(acl, encoding="utf-8")
    for secret in (password_file, folder / "acl"):
        try:
            secret.chmod(0o600)   # the broker complains about files that others can read
        except OSError:
            pass
    (folder / "mosquitto.conf").write_text(
        f"listener {port} 127.0.0.1\nallow_anonymous false\npassword_file {password_file.as_posix()}\nacl_file {(folder / 'acl').as_posix()}\npersistence false\nlog_dest file {(folder / 'broker.log').as_posix()}\n",
        encoding="utf-8")
    process = subprocess.Popen([broker, "-c", str(folder / "mosquitto.conf")], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(100):
        try:
            socket.create_connection(("127.0.0.1", port), timeout=0.2).close()
            return process, port
        except OSError:
            if process.poll() is not None:
                raise AssertionError(f"mosquitto did not start: {(folder / 'broker.log').read_text(errors='replace') if (folder / 'broker.log').exists() else ''}")
            time.sleep(0.1)
    process.kill()
    raise AssertionError("mosquitto did not open its port")


# ---- the check -------------------------------------------------------------------------------------------------------------------------------

def read_samples(binary: str | None) -> list[tuple[str, dict]]:
    if binary:
        text = subprocess.run([binary], capture_output=True, text=True, check=True).stdout
    elif sys.stdin is not None and not sys.stdin.isatty():
        text = sys.stdin.read()
    else:
        text = ""
    samples: list[tuple[str, dict]] = []
    for line in text.splitlines():
        kind, _, body = line.partition(" ")
        if not body.startswith("{"):
            continue
        try:
            payload = json.loads(body)
        except ValueError:
            continue
        if isinstance(payload, dict):
            samples.append((kind, payload))
    return samples


def project_name() -> str:
    try:
        return str(json.loads((Path(__file__).resolve().parents[1] / "armor.project.json").read_text(encoding="utf-8")).get("name", ""))
    except (OSError, ValueError):
        return ""


def run(samples: list[tuple[str, dict]]) -> int:
    name = project_name()
    prefix = IDENTITIES.get(name, "field-node")
    lines = [(token, payload, found) for token, payload in samples if (found := classify(token, payload, "bench-node")) is not None]
    node = next((found[1] for _, _, found in lines), "bench-node")
    user, password = f"{prefix}-{node}", "node-secret-for-the-test"
    other = "someone-else" if node != "someone-else" else "someone-more"
    messages = [(found[0], token, payload) for token, payload, found in lines if found[1] == node and found[2]]
    orders = sorted({found[0] for _, _, found in lines if found[1] == node and not found[2]} | {f"armor/control/{node}/command"})
    health = f"armor/health/{node}"
    acl = (f"user {user}\ntopic write armor/health/{node}\ntopic write armor/+/{node}/#\n" + "".join(f"topic read {topic}\n" for topic in orders)
           + "\nuser armor-server\ntopic readwrite armor/#\n")
    with tempfile.TemporaryDirectory() as directory:
        process, port = start_broker(Path(directory), {user: password, "armor-server": "server-secret"}, acl)
        clients: list[Client] = []
        try:
            server = Client(port, "armor-server-test", "armor-server", "server-secret")
            clients.append(server)
            server.subscribe("armor/#")

            will = json.dumps({"node_id": node, "timestamp_ms": 0, "online": False}, separators=(",", ":")).encode()
            node_client = Client(port, node, user, password, keepalive=20, will=(health, will))
            clients.append(node_client)
            node_client.publish(health, json.dumps({"node_id": node, "timestamp_ms": 5, "online": True}, separators=(",", ":")).encode(), retain=True)
            for topic in orders:
                node_client.subscribe(topic)

            # a server that joins later is told by the broker that the node is online
            late = Client(port, "armor-server-late", "armor-server", "server-secret")
            clients.append(late)
            late.subscribe(health)
            retained = late.received(health)
            assert retained and json.loads(retained[-1])["online"] is True, "a server that joins later does not find the node online"
            assert any(item[2] for item in late.inbox), "the birth message was not kept by the broker (it must be retained)"

            # everything the node's serialiser prints reaches the server, byte for byte, on the topic of its kind
            for topic, kind, payload in messages:
                body = json.dumps(payload, separators=(",", ":")).encode()
                before = len([item for item in server.inbox if item[0] == topic and item[1] == body])
                node_client.publish(topic, body)
                server.pump(0.3)
                got = len([item for item in server.inbox if item[0] == topic and item[1] == body])
                assert got == before + 1, f"the {kind} message did not reach the server once on {topic}"

            # what the server tells the node reaches it (and the node acknowledges it, which Client does for every QoS 1 message)
            for topic in orders:
                told = next((json.dumps(payload, separators=(",", ":")).encode() for _, payload, found in lines if found[0] == topic), b'{"operation":"identify"}')
                server.publish(topic, told)
                node_client.pump(0.5)
                assert [item for item in node_client.inbox if item[0] == topic and item[1] == told], f"the node never received what was sent to it on {topic}"

            # a node speaks only on its own topics
            before = len(server.inbox)
            node_client.publish(f"armor/health/{other}", b'{"node_id":"someone-else","timestamp_ms":1,"online":true}')
            node_client.publish(f"armor/node/{other}/telemetry", b"{}")
            server.pump(0.8)
            assert not [item for item in server.inbox[before:] if other in item[0]], "the broker let the node publish on the topic of another node"

            # the connection dies with no goodbye: the retained last will tells the server
            server.inbox.clear()
            node_client.drop()
            clients.remove(node_client)
            offline = server.received(health, wait=5)
            assert offline and json.loads(offline[-1])["online"] is False, "the last will of the node did not reach the server when its connection died"
            after = Client(port, "armor-server-after", "armor-server", "server-secret")
            clients.append(after)
            after.subscribe(health)
            kept = after.received(health)
            assert kept and json.loads(kept[-1])["online"] is False, "the broker does not keep the last will as the node's last state"
        finally:
            for client in clients:
                try:
                    client.close()
                except OSError:
                    pass
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
    print(f"MOSQUITTO=OK {len(messages)} messages of {name or 'a node'} through a real broker as {user}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", help="run this emit_samples binary instead of reading stdin")
    args = parser.parse_args()
    if not (find("mosquitto") and find("mosquitto_passwd")):
        if os.environ.get("ARMOR_REQUIRE_MOSQUITTO"):
            print("MOSQUITTO=MISSING the broker (mosquitto and mosquitto_passwd) is required here and is not installed", file=sys.stderr)
            return 1
        print("MOSQUITTO=SKIPPED no mosquitto on this machine (it runs in CI, where the broker is installed)")
        return 0
    try:
        return run(read_samples(args.binary))
    except AssertionError as error:
        print(f"MOSQUITTO=FAILED {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
