<p align="center">
  <img src="images/ARMOR_BANNER.svg" alt="ARMOR-NODE-RADAR banner" width="100%">
</p>

# 📡 ARMOR-NODE-RADAR

<p align="center">
  🇺🇸 <b>English</b> |
  <a href="README_spa.md">🇪🇸 Español</a> |
  <a href="README_fra.md">🇫🇷 Français</a> |
  <a href="README_ita.md">🇮🇹 Italiano</a> |
  <a href="README_deu.md">🇩🇪 Deutsch</a> |
  <a href="README_zho.md">🇨🇳 简体中文</a> |
  <a href="README_jpn.md">🇯🇵 日本語</a> |
  <a href="README_por.md">🇵🇹 Português</a>
</p>

### Field-node firmware (Waveshare ESP32-S3-ETH, three radars or presence sensors, Ethernet and Wi-Fi) with its own web panel, and its host-tested core

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Tests-7371%20checks-2ea44f.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.8.0.7-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**Honesty check - what runs today:** The radar node is the one node that exists as hardware: its firmware runs on a real Waveshare ESP32-S3-ETH board with a radar and reports to the CM5 test bench, and its web panel and over-the-air updates are in use on it. The hardware-independent core (7,371 checks in 14 test programs: the decoders of the LD2450, the LD2461 and four presence sensors, the LD2450 command channel, the settings and their checks, the pin table, users and sessions, the network plan, the message serialiser whose output ARMOR-COMMON accepts, and the release and signature checks) is tested on a computer, the **web panel** is exercised in a real browser against a stand-in node, and the **firmware image builds** in the ESP-IDF 5.5.5 container. What is **not** proven yet: the LD2461 and presence-sensor decoders match only the worked examples of their manuals, with no module behind them; the signature check of the updates and the Bluetooth PIN pairing are proven on a computer with real signatures but not yet on the board; the radar command channel is unchecked against a module; and the panel's HTTPS certificate (made by the node itself) is untried in every browser.

---

## 📋 At a glance

| | |
|---|---|
| Version | `0.8.0.7` |
| Maturity | Established |
| Role | Radar field node for ESP32-S3 (family: Field Nodes, parent: ARMOR-COMMON) |
| Stack | C++17, ESP-IDF 5.5.5, ESP32-S3 |
| Tests | 7371 checks in 14 host programs and 6 Python checks (`ctest`) |
| Boards | Waveshare ESP32-S3-ETH (Ethernet) and ESP32-S3-WROOM-1 N16R8 (Wi-Fi only) |
| Sensors | Up to 3 radars (LD2450, LD2461) or presence sensors per node, VEML7700 light sensor |
| Runs on | Waveshare ESP32-S3-ETH or ESP32-S3-WROOM-1 N16R8; host tests on any computer |
| On real hardware | Runs on a real Waveshare ESP32-S3-ETH board with a radar, reporting to the CM5 test bench |
| License | GPL-3.0-or-later |

**Jump to:** [Overview](#-overview) · [Architecture](#-architecture) · [Security](#-security--safety) · [Interfaces](#-interfaces) · [Configuration](#-configuration) · [Quick start](#-quick-start) · [Testing](#-testing) · [Troubleshooting](#-troubleshooting) · [Structure](#-repository-structure) · [Related](#-related-projects)

## 🚦 Project status

Each line is one part of this repository and the light says how far it is today. It is written from what the repository can show (tests, builds, runs), not from what is planned.

🟢 Done and verified · 🟡 Works, with known limits · 🟠 Partly done or not verified yet · 🔴 Not done or not working

| Area | State | What it means here |
|---|---|---|
| Software | 🟢 | The firmware builds in the ESP-IDF 5.5.5 container and runs on a real ESP32-S3-ETH board. |
| Tests | 🟢 | 7,371 checks in 14 host programs pass (`ctest`): the decoders, the settings, the pin table, the users, the network plan, the message serialiser and the release checks. |
| Protocols and data | 🟡 | The LD2450 runs on a real radar; the LD2461 and the four presence-sensor decoders only match the worked examples of their manuals, and the radar command channel is unchecked against a module. |
| Interface | 🟡 | The web panel is in use on the board and is exercised in a real browser against a stand-in node; its HTTPS certificate, made by the node itself, is untried in every browser. |
| Hardware | 🟡 | It is the one node that exists as hardware: a real Waveshare ESP32-S3-ETH board with a radar reports to the CM5 test bench; no other sensor module is connected. |
| Security | 🟡 | The Bluetooth PIN pairing and the update signature check are proven on a computer with real signatures but not yet on the board; there has been no independent review. |
| Updates | 🟡 | Over-the-air updates are in use on the board; the signed-update check has not run there yet. |
| Integration | 🟡 | The messages of the board reach the CM5 test bench server, and the same messages are accepted by the shared schemas and conformance vectors of ARMOR-COMMON. It has been seen on the bench only, not in an installation. |
| Documentation | 🟢 | Eight languages and the changelog are up to date. |

## 🎯 Overview

* **Two boards, one firmware:** the Waveshare ESP32-S3-ETH (Ethernet, the default) and an ESP32-S3-WROOM-1 N16R8 with no Ethernet (Wi-Fi only: its set-up asks for the Wi-Fi network to join and it always keeps its own network). The image is chosen when it is built (`tools/build_node.sh generic s3-eth` or `generic s3-wifi`); the pin table and the way in follow the board, the panel hides what the board does not have, and an image is only for its own board. Both build and are host-tested; neither has run on a board.
* **A node covers up to 270 degrees:** each Waveshare ESP32-S3-ETH reads up to three LD2450 radars on its three UARTs, mounted 75 degrees apart, over wired Ethernet (W5500) with DHCP or a fixed address, powered by PoE or USB. Studio's *Add a 270° node* creates the three radars already wired to the node.
* **A web panel on every node,** in the look of Studio and its eight languages, embedded in the firmware: overview, network, Wi-Fi, broker, radars, pins, users, firmware update and log. A node with no user opens the Wi-Fi `ARMOR-SETUP-xxxxxx` and creates its first administrator with a set-up code; passwords are salted PBKDF2, sessions are random tokens, and every setting lives in the node's flash, so one image serves every node and no password is compiled in ([the panel](docs/NODE_PANEL.md)).
* **One Wi-Fi from many nodes:** each node can offer an access point joined to its Ethernet port; give the nodes the same name and password and the channel on automatic (1, 6 or 11 by MAC) and phones and Wi-Fi sensors see one network with one DHCP server. It is not a radio mesh: every node keeps its cable. A node can instead join a router's Wi-Fi (with a search for networks), and one with no cable can be configured over **Bluetooth** from the Android app.
* **Pins for the server:** any free pin becomes an input, an output (with a safe state when the broker is lost), PWM or an analogue reading, and appears as a device of the server, so a relay or a contact needs no new firmware. The board's reserved pins are never offered.
* **Over-the-air updates with rollback** from the panel (two slots on the 16 MB flash), and a **link from Studio** to each node's panel, from the address the node publishes.
* **The panel over HTTPS:** the node makes its own certificate (self-signed, kept in flash) and serves the panel on port 443 as well as 80, or only on 443; the session cookie is marked Secure and the certificate fingerprint is shown to compare with the browser's warning.
* **Stable track identities:** each frame's targets are matched to the ones followed before, so a person keeps the same track id, a lost frame does not flicker and positions are smoothed a little.
* **Six sensor models, one per port:** a port carries an LD2450 or an LD2461 (trackers that feed the perimeter, with the model's own field in Studio) or a presence sensor (LD2410, LD2412, LD2410S, MR24HPC1) that becomes a device of the server and publishes presence and distance. Chosen in the panel; what each one is, its protocol and what was not verified are in `docs/SENSORS.md`.
* **LD2450 decoder, health and configuration:** a resynchronising framer finds frames in a noisy stream; each 30-byte frame gives up to three targets that become contract tracks; the console and the panel say per radar whether it reports, is silent or garbled. The panel can also read the module's version, choose one or three targets and set detection zones (a protocol that is unchecked against a module).
* **Ambient light and contract-exact messages:** the VEML7700 with automatic range; telemetry, health and information JSON that follow the published schemas and refuse to write anything invalid, with wall-clock timestamps (SNTP) and an MQTT last will. Telemetry is withheld while no radar reports, never an empty 'all clear'.
* **One image for every board, like a network product:** the firmware is the same and the MAC tells the boards apart (`armor-` and six digits until named); `adopt_node.py` gives a freshly flashed node its administrator, its broker identity and the fleet's shared settings over the network, with a set-up code computed from its MAC, so 5 nodes or 27 are the same work.
* **Bench tools:** flashing over USB-C, a stand-in node for working on the panel, and a converter from a log of raw frames to a test fixture ([bench bring-up](docs/BENCH_BRINGUP.md)).
* **Signed over-the-air updates:** the node installs from GitHub only an image whose SHA-256 matches and that carries a valid Ed25519 signature of the maintainer (the public key is in the firmware, the private one never leaves the maintainer); a file uploaded by hand by an administrator in the panel is still accepted. The pages and messages of the panel are in eight languages.
* **Bluetooth pairing with a PIN:** the six-digit PIN is derived from the set-up code, printed on the USB console and shown to an administrator in the panel; the phone has to enter it before it may write the node's settings.
* **Only the house may use the panel:** the panel answers the private network by default (or the networks its owner lists, such as a VPN), refuses a list that would lock out whoever saves it, counts wrong passwords for each person as well as each address, and works behind the server's proxy so that it can be reached from outside the house without publishing the node.
* **Optional encrypted link to the broker (TLS):** off by default, so the node talks plain MQTT as before. A switch and a box for the certificate of the installation's own authority (PEM, at most 2 KB) in the Broker page make it connect with `mqtts://`, trust that authority only and check that the broker's certificate was made for the address it reached; anything that is not one PEM certificate is refused. Covered by host tests; the handshake has not been run on a board yet.
* **Health report:** about once a minute the node publishes how long it has been up and why it last restarted, its free memory, its link and Wi-Fi signal, how many times it connected to the broker and the last error it logged (`armor/health/<node>`, the `node_health` contract of ARMOR-COMMON). The code that builds it is shared by every node and tested on a computer; it has not run on a board.
* **Control from the server:** Studio can restart the node cleanly, pause it for up to an hour (it resumes by itself), put its work in standby (no end, kept across restarts, the node stays connected and keeps answering), make it show itself, ask for a diagnostics snapshot and mark it as in maintenance (`armor/control/<node>/...`, the `node_control` contract of ARMOR-COMMON). Every order takes two steps with a one-time token that belongs to that operation, only the server may write them on the broker, and there is no factory reset, no erase and no deep sleep. While the node is paused or in standby it publishes no targets and no presence (the heartbeat still says it is there). The code is shared by every node and tested on a computer; it has not run on a board.
* **A perimeter engine on the node (off by default):** six detection zones and three exclusion zones, each a rectangle that can be turned by an angle, three tripwire lines that count crossings by direction, three entry paths, pet immunity and masking detection. It runs next to the sensors, so it works when the broker cannot be reached, keeps its events while the broker is away and publishes them on `armor/radar/<node>/events`; the zone editor is in the node's panel. None of it has run with a sensor on a board yet.
* **Who each sensor is, and its own Bluetooth closed:** at start the node reads the firmware version and the Bluetooth address of each sensor, closes the sensors' own Bluetooth (it asks for no PIN) by default, and can hand the serial line of one sensor to an administrator's PC tool for a few minutes (a transparent serial bridge). `GET /metrics` for administrators and an export and all-or-nothing import of the settings complete it.
* **Diagnostics without a cable:** after a start that was not clean (a crash, a watchdog, a sag of the supply, a failed allocation) the health report says why for ten minutes; a crash leaves its core dump to look at, download or erase from the panel; the stored settings are copied before every firmware install and recovered if an update damages them; and an administrator can ask the node, through the server, for a diagnostic package or its log with a one-time token.
* **Adoption over HTTPS with a pinned certificate:** `tools/adopt_node.py` accepts `https://<address>` with `--fingerprint` (the SHA-256 of the node's certificate, checked on the very connection that carries the request) or `--show-fingerprint` to read it; it warns about plain HTTP and about a fleet file with a fixed address or host name, and the panel warns too when such a file is imported.
* **The sensor reader waits for events instead of polling:** the reading task sleeps on the serial lines' event queues and drains everything that arrived, flushes a line that overflowed and counts overflows and errors per line; `system.reader_idle_ms` (100; 0 = the earlier 10 ms polling) sets how long it waits, and a queue that fails sends it back to polling by itself.
* **Optional sealing of stored secrets:** off by default (`security.seal_secrets`, or the server's `nodes.nvs_encryption`). When on, the passwords, tokens, keys, PINs and password hashes the node stores are kept sealed with ChaCha20-Poly1305 and everything else stays readable; turning it on or off rewrites them with nothing lost, even if the node stops half-way. No eFuse is burned and the flash is not encrypted: the key lives in a partition of the same chip, so it does **not** protect against someone who holds the board, and a node installed before that partition existed keeps its secrets as they were until it is flashed with the new partition table.

## 🧩 Role in A.R.M.O.R.

ARMOR-NODE-RADAR is the eyes of the perimeter. Each node reads up to three radars or presence sensors and publishes tracks, light and health to [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) over MQTT; [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) draws them on the site plan and [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) weighs them against the cameras. Its firmware shares a base with the other nodes (the panel, Bluetooth set-up, signed updates, sign-in and the clock come from the firmware base of [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), so what you learn on one node holds on all of them.

| Name | How | What for |
|---|---|---|
| ARMOR-SERVER | MQTT `armor/node/<id>/telemetry|health|info|command` | Receives the radar messages (the CM5 test bench today) and orders the node |
| ARMOR-STUDIO | link to the panel | Opens the node's panel from the address the node publishes |
| ARMOR-ANDROID-CONTROL | Bluetooth | Sets the node up in the field |
| ARMOR-COMMON | contracts, firmware base | The messages are checked against its schemas; the common firmware is copied from it |

## 🏛️ Architecture

<p align="center">
  <img src="images/architecture.svg" alt="ARMOR-NODE-RADAR architecture: who calls it, what it holds and what it depends on" width="100%">
</p>

## 🖼️ Screenshots

Captured at 1920×1080 on a test bench, with sample data and no real installation behind them.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Overview: the state of the node at a glance - network, broker, firmware and what it reports." width="100%"><br><sub>Overview: the state of the node at a glance - network, broker, firmware and what it reports.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radars: each radar of the node, its model, pins and calibration." width="100%"><br><sub>Radars: each radar of the node, its model, pins and calibration.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Pins: which pin does what, checked so that two functions never share one." width="100%"><br><sub>Pins: which pin does what, checked so that two functions never share one.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Network: Ethernet or Wi-Fi addressing of the node, set from its own web panel." width="100%"><br><sub>Network: Ethernet or Wi-Fi addressing of the node, set from its own web panel.</sub></td></tr>
</table>

## 🔒 Security & Safety

| Threat | Defence |
|---|---|
| Passwords in clear or compiled in | PBKDF2-HMAC-SHA256 with 10 000 rounds; no password in the firmware (a node with no user enters set-up); at most 4 users (admin or viewer) and 8 to 64 characters; at most 6 sessions, 30 minutes idle |
| Taking a freshly flashed node | Set-up needs a start-up code: the one of the build (`CONFIG_ARMOR_SETUP_CODE`) or a random one kept in flash and shown on the USB console; with a fleet secret it is `HMAC-SHA256(secret, MAC)`; it is also the key of the set-up Wi-Fi (WPA2, never bridged) |
| The panel reached from the internet | It answers the private networks of a house by default (10/8, 172.16/12, 192.168/16, 169.254/16, 127/8 and 100.64/10 where some VPNs live); the setting `web.admin_sources` lists other IPv4 addresses and networks (8 entries, 120 characters); anything outside is refused except the public identity `GET /api/v1/session`; a list that would lock out the person saving it is rejected |
| The panel in clear over HTTP | HTTPS on 443 with a certificate the node makes itself (self-signed, in flash, SHA-256 fingerprint shown to compare); the panel can be http, https or both; the session cookie is always HttpOnly and SameSite=Strict (and Secure over TLS) |
| A fake firmware from GitHub | An image is installed only with a matching SHA-256 and a valid Ed25519 signature made with the maintainer's key (`no_checksum`, `no_signature`, `bad_signature`, `checksum_mismatch`, `wrong_firmware`, `not_newer`, `version_mismatch` refuse it); a manual upload by an administrator is not checked: an explicit limit |
| Hijacking the Bluetooth set-up | A 6-digit PIN derived from the set-up code, shown on the USB console and to an administrator; the phone must type it before writing settings; the set-up window lasts `CONFIG_ARMOR_BLE_SETUP_MINUTES` (10 by default, 1 to 120) |
| A remote order that does harm | Two steps with a single-use token tied to that operation; only the server may write those topics to the broker; a pause lasts one hour at most; there is no factory reset, no erase and no deep sleep by remote order |
| Stored secrets read by someone who has the board | Optional (off by default, switch `security.seal_secrets` or the server's `nodes.nvs_encryption`): ChaCha20-Poly1305 for passwords, tokens, keys, PINs and hashes. It does not encrypt the flash or burn an eFuse: the key is in a partition of the same chip, so it does not protect against whoever holds the board |
| The broker link in clear | Optional TLS (`mqtts://`) with the owner's own authority (PEM up to 2 KB) and a check of the name; off by default. The handshake has only run in host tests, not on a board |
| A sensor's own Bluetooth | By default the node closes the Bluetooth of every LD sensor (it asks for no PIN); a transparent serial bridge to a computer opens only for a few minutes and only by an administrator |
| Adopting a node on an untrusted network | `tools/adopt_node.py https://<address> --fingerprint <SHA-256>` checks the certificate on the same connection; it warns about plain HTTP and about fleet files with a fixed address |
| A dangerous pin | The reserved pins of the board are never offered; an output has a safe state if the broker is lost; two functions cannot share a pin (checked) |
| A public firmware that carries a fleet secret | `CONFIG_ARMOR_FLEET_SECRET` is empty by default: the public build carries no secret |

Read [SECURITY.md](SECURITY.md) to report a vulnerability. There has been no independent review.

## 🔌 Interfaces

| Part | What it sets |
|---|---|
| MQTT | `armor/node/<id>/{telemetry,health,info,command}`, `armor/health/<id>` (its last will is `{"node_id":"..","online":false}`), `armor/control/<id>/{command,result,maintenance}` and `armor/control/all/command`, `armor/device/<id>/<pin>/{state,set}`, `armor/radar/<id>/events`, `armor/radar/<id>/control/armed` |
| Panel (HTTP 80, HTTPS 443) | Routes `session`, `login`, `logout`, `setup`, `status`, `config` (+ export, import), `wifi/scan`, `time`, `radars` (+ command), `perimeter` (+ armed, learn, zone), `pins` (+ command), `bridge` (+ open, close, io), `ota` (+ check, install, progress, copy, switch), `users` (see `account`), `ble-pin`, `log`, `coredump`, `factory-reset`, `reboot`; `/metrics` for administrators and the WebSocket `/ws/radar-map`; `tests/check_routes.py` checks that every route the panel calls exists |
| Panel texts | 76 error codes with a text in eight languages (832 rows in `panel/text.js`), checked by `tests/check_panel_text.py` |
| Hardware | 3 UARTs for LD2450, LD2461, LD2410, LD2412, LD2410S and MR24HPC1 (RX 16, 17, 18; TX 15, 21, 38); W5500 over SPI (MOSI 11, MISO 12, SCLK 13, CS 14, INT 10, RST 9, 20 MHz); VEML7700 on I2C (GPIO 1 and 2); BOOT button |

## ⚙️ Configuration

| Variable | Default | What it sets |
|---|---|---|
| `CONFIG_ARMOR_NODE_ID` | `""` | The name; empty gives `armor-` and the last three bytes of the MAC |
| `CONFIG_ARMOR_SETUP_CODE`, `CONFIG_ARMOR_FLEET_SECRET` | `""` | The set-up code of the build (empty: a random one) and the fleet secret (16+ characters; never in the repository) |
| `CONFIG_ARMOR_RADAR_COUNT` | 3 (0-3) | How many radars are connected |
| `CONFIG_ARMOR_TRACK_STABILISER` | y | Keep a track's identity from frame to frame |
| `CONFIG_ARMOR_MQTT_URI`, `_USERNAME`, `_PASSWORD` | `""` | The first broker settings (they can be changed in the panel) |
| `CONFIG_ARMOR_HEARTBEAT_S` | 10 (2-300) | Seconds between health beats |
| `CONFIG_ARMOR_TELEMETRY_PERIOD_MS` | 200 (200-5000) | At most 5 messages per second |
| `CONFIG_ARMOR_BLE_SETUP_MINUTES` | 10 (1-120) | How long the Bluetooth set-up stays open |
| `CONFIG_ARMOR_BOARD_S3_ETH` / `_S3_WIFI` | eth | The board: Waveshare ESP32-S3-ETH or ESP32-S3-WROOM-1 N16R8 (Wi-Fi only) |
| `CONFIG_ARMOR_BOOT_FACTORY_RESET` | y | Hold BOOT for 8 s in the first 30 s to start again |

The settings that are tuned live in the panel and are kept in flash (for example `system.reader_idle_ms`, `security.seal_secrets`, the failed-login limit per minute (30, 0 to 120) and the zones of the perimeter engine); the defaults of the firmware are in `sdkconfig.defaults` and `sdkconfig.board.*`. The flash of 16 MB holds `nvs` (64 KB), `otadata`, `phy_init`, two 3 MB slots `ota_0` and `ota_1`, `coredump` (128 KB) and `nvs_keys` (4 KB) (`partitions.csv`).

## 🚀 Quick start

**Requirements:** a C++17 compiler for the host tests; the ESP-IDF 5.5.5 container to build the image; a Waveshare ESP32-S3-ETH or an ESP32-S3-WROOM-1 N16R8.

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host   # host tests, -Werror
node tools/panel_mock.mjs --user admin:adminpass123   # the panel without a board
python tools/make_fleet.py                             # once: the fleet secret and the shared settings
tools/build_node.sh generic                           # ONE image for every Waveshare board (dist/generic-s3-eth.bin), in the ESP-IDF container
tools/build_node.sh generic s3-wifi                   # the same firmware for an ESP32-S3-WROOM-1 N16R8 with no Ethernet
```

```bat
tools\flash.bat generic COM5 monitor                  # each Waveshare board, once, by USB-C (add s3-wifi for the other board)
```

```bash
tools/adopt_node.py 192.168.0.181 --id perimetro-3 --fleet secrets/fleet.json --broker-ssh-host <cm5> --broker-ssh-user <user>
```

The host tests need any C++17 compiler (Linux, WSL, MSYS2). See the [bench bring-up](docs/BENCH_BRINGUP.md), the [panel](docs/NODE_PANEL.md) and the [hardware boundary](docs/HARDWARE_BOUNDARY.md).

## 🧪 Testing

```bash
cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host
build/host/emit_samples | python tests/check_contract.py
python -I tools/armor_ci_validate.py
```

14 host test programs and 6 Python checks run under `ctest`; 7371 checks counted by the host programs, built with `-Werror`.

| Part | What it covers |
|---|---|
| `test_core`, `test_sensors`, `test_family`, `test_uart_wake` | The decoders (LD2450, LD2461, four presence sensors), the framer, the command channel, the sensor families and the wake of the UART |
| `test_node`, `test_base`, `test_shared`, `test_board_wifi` | The settings and their checks, the pin table, users and sessions, the network plan, the shared base and the Wi-Fi board |
| `test_logic`, `test_perimeter`, `test_control`, `test_diag` | The pin logic, the perimeter engine, the two-step control orders and the diagnostics |
| `test_mqtt_tls`, `test_release` | The TLS link to the broker (host only) and the release and signature checks |
| `check_panel_text`, `check_task_safety`, `check_save_paths`, `check_ble_session`, `check_routes`, `check_bridge_client` | Python: the eight-language panel texts (832 rows), the tasks, the save paths, the Bluetooth session under its lock, the routes and the serial-bridge client |

The web panel is also exercised in a real browser against a stand-in node (`tools/panel_mock.mjs`). The LD2461 and presence-sensor decoders match only the worked examples of their manuals.

## 🏗️ Build & Release

`tools/build_node.sh generic` builds ONE image for every Waveshare board in the ESP-IDF 5.5.5 container, and `tools/flash.bat` writes it by USB-C; the same firmware is built for the Wi-Fi-only board with `s3-wifi`. Releases carry the image, its SHA-256 and its Ed25519 signature, one per board.

## 🩺 Troubleshooting

| You see | Because | Do |
|---|---|---|
| You lost the password or the node no longer joins the network | A node with set-up and no address after 90 s opens its own Wi-Fi `ARMOR-SETUP-xxxxxx` (open `http://192.168.4.1/`) | To start again hold BOOT for 8 seconds in the first 30 seconds after power-up (erases settings and users), or use *Factory reset* in the panel |
| An update from GitHub is refused | The release has no checksum or signature, they do not match (`e_no_checksum`, `e_no_signature`, `e_checksum_mismatch`, `e_bad_signature`), has no image for this board (`e_no_asset_for_board`) or is for another project (`e_wrong_firmware`) | Nothing changed on the node; try again or let an administrator upload the file by hand |
| The radar commands answer `no_answer`, `refused` or `disabled` | The radar's TX wire is not connected, the module hung, or the node closes its Bluetooth first | Check the wiring (see the bench bring-up), restart the module from the panel |
| The node shows no sensor 3 or lets a radar sleep | The module stopped talking (silence with no bad frames) | Use *restart module* in the radars page, then close the sensor's own Bluetooth |

## 📂 Repository Structure

<p align="center">
  <img src="images/repository-structure.svg" alt="Repository structure of ARMOR-NODE-RADAR" width="100%">
</p>

<details>
<summary>Text version of the tree</summary>

```text
ARMOR-NODE-RADAR/
├── main/                                the firmware: an ESP-IDF component
├── panel/                               the web panel embedded in the firmware
├── tests/                               host tests and Python checks
├── docs/                                7 guides for the bench and the panel
├── tools/                               build, flash, adopt, fleet and check tools
├── .github/                             CI workflow, issue and pull-request templates
├── images/                              banner, icon and the two pictures of this README
├── build.sh build.bat                   build through the shared project workflow
├── test.sh test.bat                     run the tests through the shared workflow
├── run.sh run.bat                       start through the shared project workflow
├── armor.project.json                   manifest: version, role, maturity, build command
├── CHANGELOG.md                         what changed in each version
├── SECURITY.md SUPPORT.md LICENSE       security policy, support and license
├── CONTRIBUTING.md CODE_OF_CONDUCT.md   how to contribute and the code of conduct
├── CMakeLists.txt                       the ESP-IDF project
├── partitions.csv                       the flash table
├── sdkconfig.defaults                   the firmware settings
├── sdkconfig.board.s3-eth               the Ethernet board
├── sdkconfig.board.s3-wifi              the Wi-Fi board
├── secrets/                             fleet and node file examples
├── build-test.sh build-test.bat         run the tests through the shared workflow
└── .gitattributes .gitignore            Git settings
```

</details>

## 🗺️ Roadmap & Known limits

**Known limits today**

* The LD2461 and presence-sensor decoders match only the worked examples of their manuals, with no module behind them; the radar command channel is unchecked against a module other than the LD2450.
* The signature check of the updates and the Bluetooth PIN pairing are proven on a computer with real signatures but not yet on the board; the TLS handshake with the broker has not run on a board.
* The health report and the control from the server have not run on the board; the perimeter engine has never run with a sensor; the panel's own HTTPS certificate is untried in every browser; the Wi-Fi board has no physical board (only the Ethernet board exists).

**Next**

* Run the perimeter engine, the health report and the control orders with real sensors and the CM5 test bench; try the TLS link to the broker and the signed update on the board.

## 🌍 Languages

README and texts exist in English, Spanish, French, Italian, German, Chinese, Japanese and European Portuguese.

## 🔗 Related Projects

**A.R.M.O.R.** (Autonomous Radar & Multimodal Observation Range) is a perimeter-security system made of independent repositories. Each one has its own version, its own tests and its own README; this is the family:

* **[ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)** - Message contracts, validators, conformance vectors and generated types
* **ARMOR-NODE-RADAR** (this repository) - Field-node firmware for ESP32-S3 with three radars and its own web panel
* **[ARMOR-NODE-SOLAR](https://github.com/JuanenRac/ARMOR-NODE-SOLAR)** - Solar inverter and battery protocols and the messages of a gateway node
* **[ARMOR-NODE-ELECTRICAL](https://github.com/JuanenRac/ARMOR-NODE-ELECTRICAL)** - Electrical node: meters, the message of the network's readings and the rules for switching
* **[ARMOR-NODE-ALARM](https://github.com/JuanenRac/ARMOR-NODE-ALARM)** - Alarm node and panel: zones, arming, delays, siren and PIN, with the server or without it
* **[ARMOR-NODE-HMI](https://github.com/JuanenRac/ARMOR-NODE-HMI)** - Touch panel: the state of the system on a wall screen, arming and acknowledging, and the home of the voice assistant
* **[ARMOR-NETWORK](https://github.com/JuanenRac/ARMOR-NETWORK)** - The local network: its devices, the internet and what changes
* **[ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER)** - Central coordinator: telemetry, alarms, devices, solar readings and cameras
* **[ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO)** - Web console: cameras, radar, alarms, solar energy and the 2D/3D site designer
* **[ARMOR-ANDROID-CONTROL](https://github.com/JuanenRac/ARMOR-ANDROID-CONTROL)** - Android operator client with a live 2D/3D radar
* **[ARMOR-IOS-CONTROL](https://github.com/JuanenRac/ARMOR-IOS-CONTROL)** - iPhone operator client
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Visual inference policy that explains its decisions and never actuates
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Camera zones, masks and detection rules for the Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Offline voice intents with a confirmation that cannot be forged
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Enclosures, electronics and the bench acceptance matrix
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Deployment, the CM5 test bench, backup and TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Offline telemetry simulator with repeatable faults
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Detects, installs and updates the ecosystem's own repositories
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architecture, security baseline and the capability matrix

## 📚 Documentation

* [Node panel](docs/NODE_PANEL.md): first start, users, network, the panel pages
* [Set-up code](docs/SETUP_CODE.md) and [Bluetooth provisioning](docs/BLE_PROVISIONING.md)
* [Sensors](docs/SENSORS.md): the six models and their decoders
* [Bench bring-up](docs/BENCH_BRINGUP.md), [bench tests](docs/BENCH_TESTS.md) and [hardware boundary](docs/HARDWARE_BOUNDARY.md)
* [Capability matrix: what is proven and what is not](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Project catalogue: versions and how the repositories depend on each other](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Changelog of this repository](CHANGELOG.md)

## 🤝 Contributing & Community

* Before a pull request run `ctest --test-dir build/host` and `python -I tools/armor_ci_validate.py`: it must pass. See [CONTRIBUTING.md](CONTRIBUTING.md) and the [code of conduct](CODE_OF_CONDUCT.md).
* The README exists in eight languages and is generated from the sources in ARMOR-DOCS: a change to the text is made there.
* A change to a message between components starts in ARMOR-COMMON.
* Do not describe as working on a board what has only been built.
* Report a vulnerability privately: [SECURITY.md](SECURITY.md). Questions, ideas and reports: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTHOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENSE

GPL-3.0-or-later - see [LICENSE](LICENSE).
