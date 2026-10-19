# Hardware boundary

## What is tested, and where

| Part | Where it runs | Evidence |
|---|---|---|
| The core in `main/core` (framer, LD2450 decoder and command channel, track set, message serialiser, dew point and heater, day/night, static map, node id, JSON reader and writer, settings and their checks, pin table, network plan, panel users and sessions, mapped-pin logic) | Any computer with CMake and a C++17 compiler | `tests/test_core.cpp` and `tests/test_node.cpp` (`-Werror`) |
| The firmware's JSON against the published contract | A computer | `tests/emit_samples.cpp` piped into `tests/check_contract.py`, validated by ARMOR-COMMON |
| The web panel (pages, eight languages, login, set-up, saving, field problems, pins, radars, phone width) | A real browser (headless Edge) against `tools/panel_mock.mjs`, a stand-in for a node | a scripted session; no text key left untranslated, no console error |
| `main/*.cpp` outside `core` (UARTs, W5500, Wi-Fi and the bridge, HTTP server, OTA, NVS, PWM, ADC, I2C, MQTT, SNTP, tasks) | ESP-IDF 5.4 | **builds** in the ESP-IDF container (`tools/build_node.sh`); **runs on two real ESP32-S3-ETH nodes**: the HTTP and HTTPS panel, users, settings in flash, Ethernet, MQTT with an account per node, telemetry, the live map and the over-the-air update. Not yet recorded on a node: the Wi-Fi access point bridged to the cable, the light sensor, mapped pins and the PoE current |

Run `build-test` from ARMOR-COMMON's launcher (or the commands in the README) for the host part. Without ESP-IDF the run states that the
firmware image was **not** built; `tools/build_node.sh` builds it in the container. The first day on a board is
[BENCH_BRINGUP.md](BENCH_BRINGUP.md); the panel is described in [NODE_PANEL.md](NODE_PANEL.md).

## The two boards

The firmware is built for two boards, chosen when the image is built (`tools/build_node.sh NODE BOARD`, `dist/NODE-BOARD.bin`; the host tests build both profiles). **An image is for ONE board.**

| Profile | Board | Way in | Pins |
| --- | --- | --- | --- |
| `s3-eth` (the default) | Waveshare ESP32-S3-ETH | The cable (DHCP or fixed), the Wi-Fi access point (bridged or on its own), or Wi-Fi | GPIO 9 to 14 (W5500) and 8 (camera) reserved, 4 to 7 the microSD socket |
| `s3-wifi` | ESP32-S3-WROOM-1 N16R8 (16 MB flash, 8 MB octal PSRAM, two USB-C sockets), no Ethernet | Wi-Fi only: a station, plus the node's own network | GPIO 4 to 14 free; 43, 44 (the USB-serial socket) and 48 (the RGB LED) with a warning |

The `s3-wifi` image has no W5500 driver and no bridge, and the panel hides what the board does not have. The three radars, the light sensor and the mapped pins use the same default pins on both boards. **Nothing about the `s3-wifi` board has been tried.**

## The Waveshare board

The target is the **Waveshare ESP32-S3-ETH**: ESP32-S3R8 (dual core, 240 MHz, **8 MB of octal PSRAM**), a **16 MB** flash, the W5500 Ethernet
(RJ45, 10/100), a USB-C port (native USB: the log and the flashing), a microSD socket, a camera connector, a ceramic antenna (an IPEX
connector is available by moving a resistor), and PoE through a separate module on the board's PoE header. Consequences in the firmware:

* the flash is split into two application slots for over-the-air updates with rollback (`partitions.csv`), and the settings store;
* the PSRAM is enabled but **optional**: if it does not answer, the node starts without it (a wrong guess must not brick a board);
* GPIO 33 to 37 (PSRAM), 26 to 32 (flash), 19 and 20 (USB), 9 to 14 (Ethernet) and 8 (camera) are never offered as pins; 0, 3, 45 and 46
  (read while the chip starts) come with a warning; 4 to 7 are the microSD socket. This table is `main/core/board_pins.hpp`, written from the
  manufacturer's pin map and the chip's datasheet. GPIO 33 and 34 are kept out although the datasheet is only certain about 35 to 37;
* the camera connector shares its pins with the header: which ones is not in the documents at hand, so a wiring that uses a camera must be
  checked against the schematic;
* the radars get 5 V from VSYS, not VBUS, and the PoE module's current has not been measured.

## The three radars

The three LD2450 modules need independent hardware UARTs, so all three ESP32-S3 UARTs are used (the chip has three, so no software UART is
needed). UART0 is the default console, therefore `sdkconfig.defaults` moves the console to the native USB Serial/JTAG peripheral. Confirm
on the real board that its USB connector is wired to the ESP32-S3 native USB before relying on this. The pins are settings of the node: RX
16, 17, 18 and TX 15, 21, 38 by default; the node refuses settings where a pin repeats or clashes with a reserved one.

The VEML7700 is read on I2C (GPIO 1 and 2 by default), its interrupt pin is not used. The AHT20 is not read: the message contract has no
field for it. The node fails closed: without a light value it withholds telemetry, unless the settings hold an explicit bench value.

## The LD2450: report frame and command channel

`main/core/frame_framer.hpp` recovers frames from a noisy byte stream (header, length, tail, resynchronisation) and knows nothing about
their content. `main/core/ld2450.hpp` is the decoder for the **HLK-LD2450** report frame, written from the Hi-Link *Instruction manual
V1.00*, section 6, and nothing else: UART 256000 8N1, a 30-byte frame with the header `AA FF 03 00`, three 8-byte targets (x, y, speed,
distance resolution; bit 15 set means positive) and the tail `55 CC`, ten times a second. The manual's own worked example is a test and
decodes to exactly the values the manual gives (x = -782 mm, y = 1713 mm, speed = -16 cm/s, gate 320 mm), and the JSON built from it is
accepted by ARMOR-COMMON.

`main/core/ld2450_command.hpp` builds the **configuration commands** and reads their acknowledgements (enter and leave configuration, one or
three targets, zones, Bluetooth, restart, factory reset, firmware version, the serial speed). That channel is **not in the manual above**: it
follows the manufacturer's separate serial protocol as it is publicly documented and as community drivers implement it. It has not been
checked against the manufacturer's document nor against a module, the panel says so, shows the module's raw answer, and offers no change
of the serial speed (a wrong index would leave the module unreachable).

What the manual does **not** say, and the code therefore does not claim:

* whether a target keeps its slot (1 to 3) from frame to frame: the slot number is used as the track id only because nothing better exists,
  so a track's identity across frames is **unproven**;
* the orientation of the axes beyond the example (the manual shows a wall-mounted radar with a detection range of 6 m and an azimuth of plus
  or minus 60 degrees);
* anything about the **LD2461**: the documents available are for the LD2450 only.

No frame has been captured from a real module yet. The first thing to do on the bench is to record real UART bytes into a test fixture and
check them against this decoder, and to read the module's firmware version from the panel.

The firmware withholds telemetry until an ambient-light reading exists, because the contract requires `lux` and a made-up value would mislead
the server's day/night decisions. It also withholds it while no radar is reporting, so silent radars never look like a quiet perimeter.

## Identity, time and offline detection

* The identity of a node is a setting (`armor-` and six digits of its MAC until it is changed); every node needs its own identity and broker ACL.
* Message timestamps are wall-clock milliseconds from SNTP, not uptime: the server ignores a message older than the last one it accepted from a
  node, so an uptime clock would silence a rebooted node for as long as it had run before. Before its clock is set the node still publishes, with the time since it started, and the server stamps what it receives with its own time.
* The MQTT last will announces `online: false` on the node's health topic if it disappears. Its timestamp cannot be newer than the node's last
  message, so the server always applies an offline message and never lets it move the stored time backwards.
* An output pin whose safe state is set falls back to it when the broker has been unreachable for the time chosen; it stays there until the server
  sends a command.

## Not decided or not proven

The PoE power path and its current, the TLS trust anchor (a CA certificate at `certs/ca.pem` is embedded when it exists) and TLS for the panel,
real captured radar frames, the command channel against a real module, the LD2461, the Wi-Fi access point's throughput and roaming, the bridge
between the wire and the access point on a real board, and everything that needs the physical board. See
[ARMOR-HARDWARE](https://github.com/JuanenRac/ARMOR-HARDWARE) for the enclosure and its validation boundary.

## Memory

The board has two kinds of memory: some 300 KB of internal memory, which the radio, the drivers and every task stack share, and 8 MB of external RAM (PSRAM) that nothing may touch while the flash is being written. The firmware keeps the internal memory for what needs it and puts the rest outside:

* **TLS** (the HTTPS panel, the update from GitHub, an encrypted broker link) takes its memory from the external RAM and, on a board where it is missing or full, from the internal memory (`CONFIG_MBEDTLS_CUSTOM_MEM_ALLOC` with the allocator of `main/psram_mem.cpp`; the stock "external" mode would fail every request on a board without PSRAM).
* **Requests above 4096 bytes** (`CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL`) go to the external RAM first: the texts and tables of the panel, the JSON documents. 32 KB of the internal memory stay reserved for what must be there.
* **Task stacks**: the tasks that only read counters and queue messages, and never touch the flash, have their stack in the external RAM (`memory::start_task`): the check for a newer release (12 KB), the broker link's sender and watchdog, the health report, the radar map of the panel and the heartbeat and telemetry of the broker link. The tasks of the radio, the network, the settings, the update itself and the equipment stay on internal stacks.
* **Seen in the log**: every minute the line `memory: internal N KB free (least N KB, biggest block N KB), PSRAM N KB free of N KB, N task stacks (N KB) in PSRAM` (a warning when the internal memory is under 40 KB), and in the status of the panel and in the health report `heap_internal_free`, `heap_internal_min`, `heap_internal_largest` and `psram_free`. The figure `heap_free` adds the external RAM, so it never looks short; judge the memory by the internal figures.

None of it has been measured on a board yet: compare the line of the log before and after the update on the same node, with the same radars and the panel open, and expect more internal memory free than before.

## Self-protection

A node that runs for months has to notice when it is in trouble and put it right by itself, without turning into a restart loop. The rules are pure functions in `core/node_guard.hpp` and `core/ble_window.hpp` (tested on a computer in `tests/test_base.cpp`); `main/node_guard.cpp` reads the real values and acts. All of it is shared by the five nodes (`ARMOR-COMMON/firmware_base`). The thresholds are starting values, to be compared with the readings of a real board.

* **The tasks the node needs exist before Bluetooth starts.** The broker link's tasks (and, in the radar node, the heartbeat and the telemetry) are made at start-up, not when the network comes up, and Bluetooth is the last thing `app_main` starts. Before, the Bluetooth stack took the internal memory first and the telemetry task was never created: the node ran for days publishing a few dozen messages. The tasks that only build and queue messages and never touch the flash have their stack in the external RAM.
* **Bluetooth memory.** The host stack of NimBLE is allocated in the external RAM (`CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL`) and the controller is given the three activities it uses (`CONFIG_BT_CTRL_BLE_MAX_ACT`). Bluetooth is not started at all when less than 40 KB of internal memory is free, and the log says the internal memory before and after it starts.
* **The set-up window closes.** In mode `setup` Bluetooth switches itself off after `CONFIG_ARMOR_BLE_SETUP_MINUTES` minutes (10 by default, 1 to 120) with nobody connected, or two minutes after a node that already has users gets an address; the stack is stopped and its memory freed, and it stays off until the node is restarted. Mode `always` is the owner's own choice and never closes.
* **The guard.** Every two seconds it looks at the tasks the node registered as needed and at the internal memory. If a task is missing, or the internal memory stays under 16 KB (back to normal over 24 KB), for a minute, it logs it (the line is also the last error of the health report, which is sent at once) and restarts the node once, cleanly. At most three such restarts in an hour of running time: after the third the node stays up and repeats the alarm every ten minutes. After a restart the guard asked for, Bluetooth stays off for that start.
* **The gate at the door of the panel.** While the internal memory is under 28 KB, or its biggest block under 12 KB, a new connection to the panel is refused before anything is allocated for it; it opens again over 40 KB and 16 KB. Open connections, the broker link and the node's own work are not touched; the log says each change with the memory at that moment.
* **A failed allocation** is not left to abort: the incident is written in memory that survives a restart and the node restarts at once; the next start logs `oom heap=<free>/<largest> after <s> s` (the last error of the health report). During the write of a firmware image, or a second time within a minute of running time, it aborts as before so that a loop stays visible.
* **The dead-man switch.** With a broker set up and connected, ten minutes without handing any message to it restart the node (same limit of three an hour); after that it stays up, degraded, and says so.
* **The guard and the server's orders.** A node the server paused or put in standby is not restarted for a task it stopped on purpose (the memory rules stay in force), and neither is one with a restart order on its way; a node kept in standby boots into it again after a restart of the guard, because the standby is kept in flash. A restart ordered on purpose leaves a mark in the memory that survives a software reset: the next start says in the log that it was not the guard's, and it is not counted against the three an hour. The planned-maintenance signal goes out before an ordered restart, a firmware install or a panel upload, so the server holds back the node-offline alert for that bounded time; a restart of the guard does not announce itself.
* **Last will and birth.** The broker link's last will on `armor/health/<node>` is retained, and the node says `{"node_id": "<node>", "online": true}` (retained) each time it connects; the radar node does the same on `armor/node/<node>/health`. The last message of the topic is always the state of the node, also for a server that starts later.
* **Building.** A cached `sdkconfig` keeps the old value of a line added to `sdkconfig.defaults`: delete `.compile/<profile>` (or `build/<profile>`) before building, and run `python tools/check_sdkconfig.py` afterwards; it lists every line of `sdkconfig.defaults` that the build did not take.

None of it has run on a board. To check on one: the log at start (`starting Bluetooth with N KB`, `Bluetooth is up`, `Bluetooth is off`), the minute line of the memory, and a forced shortage of memory to see the gate and the guard act.
