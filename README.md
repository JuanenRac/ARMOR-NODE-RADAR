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
  <img src="https://img.shields.io/badge/License-GPL%203.0-blue.svg" alt="GPL 3.0">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599c.svg" alt="Language">
  <img src="https://img.shields.io/badge/Target-ESP32--S3-e7352c.svg" alt="Target">
  <img src="https://img.shields.io/badge/Version-0.8.0.0-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-established-2EA44F.svg" alt="Maturity">
</p>

---

**Honesty check - what runs today:** The radar node is the one node that exists as hardware: its firmware runs on a real Waveshare ESP32-S3-ETH board with a radar and reports to the CM5 test bench, and its web panel and over-the-air updates are in use on it. The hardware-independent core (4,633 checks in 13 test programs: the decoders of the LD2450, the LD2461 and four presence sensors, the LD2450 command channel, the settings and their checks, the pin table, users and sessions, the network plan, the message serialiser whose output ARMOR-COMMON accepts, and the release and signature checks) is tested on a computer, the **web panel** is exercised in a real browser against a stand-in node, and the **firmware image builds** in the ESP-IDF 5.5.5 container. What is **not** proven yet: the LD2461 and presence-sensor decoders match only the worked examples of their manuals, with no module behind them; the signature check of the updates and the Bluetooth PIN pairing are proven on a computer with real signatures but not yet on the board; the radar command channel is unchecked against a module; and the panel's HTTPS certificate (made by the node itself) is untried in every browser.

---

## 📋 At a glance

| | |
|---|---|
| Version | `0.8.0.0` |
| Maturity | Established |
| Tests | 4,633 checks in 13 programs (`ctest`) |
| Stack | ESP-IDF C++ |
| On real hardware | Runs on a real Waveshare ESP32-S3-ETH board with a radar, reporting to the CM5 test bench |

## 🎯 Overview

* **Two boards, one firmware:** the Waveshare ESP32-S3-ETH (Ethernet, the default) and an ESP32-S3-WROOM-1 N16R8 with no Ethernet (Wi-Fi only: its set-up asks for the Wi-Fi network to join and it always keeps its own network). The image is chosen when it is built (`tools/build_node.sh generic s3-eth` or `generic s3-wifi`); the pin table and the way in follow the board, the panel hides what the board does not have, and an image is only for its own board. Both build and are host-tested; neither has run on a board.
* **A node covers up to 270 degrees:** each Waveshare ESP32-S3-ETH reads up to three LD2450 radars on its three UARTs, mounted 75 degrees apart, over wired Ethernet (W5500) with DHCP or a fixed address, powered by PoE or USB. Studio's *Add a 270° node* creates the three radars already wired to the node.
* **A web panel on every node,** in the look of Studio and its eight languages, embedded in the firmware: overview, network, Wi-Fi, broker, radars, pins, users, firmware update and log. A node with no user opens the Wi-Fi `ARMOR-SETUP-xxxxxx` and creates its first administrator with a set-up code; passwords are salted PBKDF2, sessions are random tokens, and every setting lives in the node's flash, so one image serves every node and no password is compiled in ([the panel](docs/NODE_PANEL.md)).
* **One Wi-Fi from many nodes:** each node can offer an access point joined to its Ethernet port; give the nodes the same name and password and the channel on automatic (1, 6 or 11 by MAC) and phones and Wi-Fi sensors see one network with one DHCP server. It is not a radio mesh: every node keeps its cable. A node can instead join a router's Wi-Fi (with a search for networks), and one with no cable can be configured over **Bluetooth** from the Android app.
* **Pins for the server:** any free pin becomes an input, an output (with a safe state when the broker is lost), PWM or an analogue reading, and appears as a device of the server, so a relay or a contact needs no new firmware. The board's reserved pins are never offered.
* **Over-the-air updates with rollback** from the panel (two slots on the 16 MB flash), and a **link from Studio** to each node's panel, from the address the node publishes.
* **The panel over HTTPS:** the node makes its own certificate (self-signed, kept in flash) and serves the panel on port 443 as well as 80, or only on 443; the session cookie is marked Secure and the certificate fingerprint is shown to compare with the browser's warning. **Stable track identities:** each frame's targets are matched to the ones followed before, so a person keeps the same track id, a lost frame does not flicker and positions are smoothed a little.
* **Six sensor models, one per port:** a port carries an LD2450 or an LD2461 (trackers that feed the perimeter, with the model's own field in Studio) or a presence sensor (LD2410, LD2412, LD2410S, MR24HPC1) that becomes a device of the server and publishes presence and distance. Chosen in the panel; what each one is, its protocol and what was not verified are in `docs/SENSORS.md`.
* **LD2450 decoder, health and configuration:** a resynchronising framer finds frames in a noisy stream; each 30-byte frame gives up to three targets that become contract tracks; the console and the panel say per radar whether it reports, is silent or garbled. The panel can also read the module's version, choose one or three targets and set detection zones (a protocol that is unchecked against a module).
* **Ambient light and contract-exact messages:** the VEML7700 with automatic range; telemetry, health and information JSON that follow the published schemas and refuse to write anything invalid, with wall-clock timestamps (SNTP) and an MQTT last will. Telemetry is withheld while no radar reports, never an empty 'all clear'.
* **One image for every board, like a network product:** the firmware is the same and the MAC tells the boards apart (`armor-` and six digits until named); `adopt_node.py` gives a freshly flashed node its administrator, its broker identity and the fleet's shared settings over the network, with a set-up code computed from its MAC, so 5 nodes or 27 are the same work. **Bench tools:** flashing over USB-C, a stand-in node for working on the panel, and a converter from a log of raw frames to a test fixture ([bench bring-up](docs/BENCH_BRINGUP.md)).
* **Signed over-the-air updates:** the node installs from GitHub only an image whose SHA-256 matches and that carries a valid Ed25519 signature of the maintainer (the public key is in the firmware, the private one never leaves the maintainer); a file uploaded by hand by an administrator in the panel is still accepted. The pages and messages of the panel are in eight languages.
* **Bluetooth pairing with a PIN:** the six-digit PIN is derived from the set-up code, printed on the USB console and shown to an administrator in the panel; the phone has to enter it before it may write the node's settings.
* **Only the house may use the panel:** the panel answers the private network by default (or the networks its owner lists, such as a VPN), refuses a list that would lock out whoever saves it, counts wrong passwords for each person as well as each address, and works behind the server's proxy so that it can be reached from outside the house without publishing the node.
* **Optional encrypted link to the broker (TLS):** off by default, so the node talks plain MQTT as before. A switch and a box for the certificate of the installation's own authority (PEM, at most 2 KB) in the Broker page make it connect with `mqtts://`, trust that authority only and check that the broker's certificate was made for the address it reached; anything that is not one PEM certificate is refused. Covered by host tests; the handshake has not been run on a board yet.
* **Health report:** about once a minute the node publishes how long it has been up and why it last restarted, its free memory, its link and Wi-Fi signal, how many times it connected to the broker and the last error it logged (`armor/health/<node>`, the `node_health` contract of ARMOR-COMMON). The code that builds it is shared by every node and tested on a computer; it has not run on a board.
* **Control from the server:** Studio can restart the node cleanly, pause it for up to an hour (it resumes by itself), put its work in standby (no end, kept across restarts, the node stays connected and keeps answering), make it show itself, ask for a diagnostics snapshot and mark it as in maintenance (`armor/control/<node>/...`, the `node_control` contract of ARMOR-COMMON). Every order takes two steps with a one-time token that belongs to that operation, only the server may write them on the broker, and there is no factory reset, no erase and no deep sleep. While the node is paused or in standby it publishes no targets and no presence (the heartbeat still says it is there). The code is shared by every node and tested on a computer; it has not run on a board.
* **A perimeter engine on the node (off by default):** six detection zones and three exclusion zones, each a rectangle that can be turned by an angle, three tripwire lines that count crossings by direction, three entry paths, pet immunity and masking detection. It runs next to the sensors, so it works when the broker cannot be reached, keeps its events while the broker is away and publishes them on `armor/radar/<node>/events`; the zone editor is in the node's panel. None of it has run with a sensor on a board yet.
* **Who each sensor is, and its own Bluetooth closed:** at start the node reads the firmware version and the Bluetooth address of each sensor, closes the sensors' own Bluetooth (it asks for no PIN) by default, and can hand the serial line of one sensor to an administrator's PC tool for a few minutes (a transparent serial bridge). `GET /metrics` for administrators and an export and all-or-nothing import of the settings complete it.
* **Diagnostics without a cable:** after a start that was not clean (a crash, a watchdog, a sag of the supply, a failed allocation) the health report says why for ten minutes; a crash leaves its core dump to look at, download or erase from the panel; the stored settings are copied before every firmware install and recovered if an update damages them; and an administrator can ask the node, through the server, for a diagnostic package or its log with a one-time token.

## 🖼️ Screenshots

Captured at 1920×1080 on a test bench, with sample data and no real installation behind them.

<table>
<tr><td width="50%" valign="top"><img src="images/screenshots/overview.png" alt="Overview: the state of the node at a glance - network, broker, firmware and what it reports." width="100%"><br><sub>Overview: the state of the node at a glance - network, broker, firmware and what it reports.</sub></td><td width="50%" valign="top"><img src="images/screenshots/radars.png" alt="Radars: each radar of the node, its model, pins and calibration." width="100%"><br><sub>Radars: each radar of the node, its model, pins and calibration.</sub></td></tr>
<tr><td width="50%" valign="top"><img src="images/screenshots/pins.png" alt="Pins: which pin does what, checked so that two functions never share one." width="100%"><br><sub>Pins: which pin does what, checked so that two functions never share one.</sub></td><td width="50%" valign="top"><img src="images/screenshots/network.png" alt="Network: Ethernet or Wi-Fi addressing of the node, set from its own web panel." width="100%"><br><sub>Network: Ethernet or Wi-Fi addressing of the node, set from its own web panel.</sub></td></tr>
</table>

## 🧩 Role in A.R.M.O.R.

ARMOR-NODE-RADAR is the eyes of the perimeter. Each node reads up to three radars or presence sensors and publishes tracks, light and health to [ARMOR-SERVER](https://github.com/JuanenRac/ARMOR-SERVER) over MQTT; [ARMOR-STUDIO](https://github.com/JuanenRac/ARMOR-STUDIO) draws them on the site plan and [ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI) weighs them against the cameras. Its firmware shares a base with the other nodes (the panel, Bluetooth set-up, signed updates, sign-in and the clock come from the firmware base of [ARMOR-COMMON](https://github.com/JuanenRac/ARMOR-COMMON)), so what you learn on one node holds on all of them.

## 📂 Repository Structure

```text
ARMOR-NODE-RADAR/
├── main/
│   ├── app_main.cpp        start-up: settings, radars, pins, network, panel, broker, Bluetooth
│   ├── node_store.cpp      settings and users in flash
│   ├── network.cpp         Ethernet, Wi-Fi access point and bridge, station
│   ├── web_server.cpp      the panel and its JSON API, login, update
│   ├── api_shared.cpp      the operations the panel and Bluetooth share (status, settings, Wi-Fi scan)
│   ├── ble_provision.cpp   configuration from a phone over Bluetooth (NimBLE)
│   ├── radar_manager.cpp   UARTs, frames, health, command channel (armor_radar.cpp, radar_tracks.hpp)
│   ├── gpio_manager.cpp    the pins mapped for the server
│   ├── mqtt_link.cpp       clock, health, telemetry, information, pin topics
│   ├── board_ethernet.cpp, light_sensor.cpp, tls_cert.cpp, log_buffer.cpp, entropy.cpp
│   ├── Kconfig.projbuild   the first settings of a build
│   └── core/               framer, ld2450, ld2450_command, ld2461, presence, sensor_model, node_config, board_pins, network_plan, auth, ble_frame, ble_dispatch, gpio_logic, json, veml7700, telemetry_json...
├── panel/                  index.html, app.js, text.js (8 languages), style.css
├── tests/                  test_core.cpp, test_node.cpp, test_mqtt_tls.cpp, test_sensors.cpp, test_board_wifi.cpp, emit_samples.cpp, check_contract.py, test_tools.py
├── tools/                  build_node.sh, make_fleet.py, adopt_node.py, provision_node.sh, flash.bat, pack_panel.py, panel_mock.mjs, frames_to_fixture.py
├── secrets/                node.conf.example, fleet.example.json (the real files are git-ignored)
├── partitions.csv, sdkconfig.defaults, sdkconfig.board.*
└── docs/                   BENCH_BRINGUP.md, NODE_PANEL.md, BLE_PROVISIONING.md, SENSORS.md, HARDWARE_BOUNDARY.md
```

## 🛠️ Development Environment

**Requirements:** a C++17 compiler for the host tests, the ESP-IDF 5.5.5 container to build the image, a Waveshare ESP32-S3-ETH or an ESP32-S3-WROOM-1 N16R8

```bash
cmake -S tests -B build/host && cmake --build build/host
build/host/test_core && build/host/test_node && build/host/test_sensors && build/host/test_board_wifi && build/host/test_release   # 4,633 checks, -Werror
build/host/emit_samples | python tests/check_contract.py
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
* **[ARMOR-SERVER-AI](https://github.com/JuanenRac/ARMOR-SERVER-AI)** - Visual inference policy that explains its decisions and never actuates
* **[ARMOR-VISION](https://github.com/JuanenRac/ARMOR-VISION)** - Camera zones, masks and detection rules for the Jetson Orin NX
* **[ARMOR-VOICE-AI](https://github.com/JuanenRac/ARMOR-VOICE-AI)** - Offline voice intents with a confirmation that cannot be forged
* **[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE)** - Enclosures, electronics and the bench acceptance matrix
* **[ARMOR-DEVOPS](https://github.com/JuanenRac/ARMOR-DEVOPS)** - Deployment, the CM5 test bench, backup and TLS
* **[ARMOR-SIMULATOR](https://github.com/JuanenRac/ARMOR-SIMULATOR)** - Offline telemetry simulator with repeatable faults
* **[ARMOR-UPDATER](https://github.com/JuanenRac/ARMOR-UPDATER)** - Detects, installs and updates the ecosystem's own repositories
* **[ARMOR-DOCS](https://github.com/JuanenRac/ARMOR-DOCS)** - Architecture, security baseline and the capability matrix

## 📚 Documentation & Community

Where to read more:

* [Capability matrix: what is proven and what is not](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/CAPABILITY_MATRIX.md)
* [Project catalogue: versions and how the repositories depend on each other](https://github.com/JuanenRac/ARMOR-DOCS/blob/main/docs/PROJECT_CATALOG.md)
* [Changelog of this repository](CHANGELOG.md)
* [License (GPL-3.0-or-later)](LICENSE)
* Questions, ideas and reports: electrohobby3d@gmail.com

## 👤 AUTHOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENSE

GPL-3.0-or-later - see [LICENSE](LICENSE).
