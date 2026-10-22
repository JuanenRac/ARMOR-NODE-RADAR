# The sensors a node can carry

A node has three serial ports. Each one can carry one of six millimetre-wave sensors; the model is chosen per port in the panel (Sensors →
Radars) or in the settings (`radars[].model`). Everything below comes from the manufacturers' documents in the project's `sensors` folder;
the worked examples in those documents are the unit tests (`tests/test_sensors.cpp`). **No sensor has been connected to a node yet**, so
nothing here has been seen working on hardware.

| Model (`model`) | Kind | Range and field (manufacturer's figures) | Serial default | Node commands |
| --- | --- | --- | --- | --- |
| HLK-LD2450 (`ld2450`) | tracker, 3 targets | 6 m, ±60° | 256000 | read information, one or three targets, zones, Bluetooth, restart, factory reset |
| HLK-LD2461 (`ld2461`) | tracker, 5 tracks | 8 m for people moving, ±45° | 9600 | read information, zones, factory reset |
| HLK-LD2410B / LD2410C (`ld2410`) | presence + distance | about 6 m | 256000 | read information, Bluetooth, restart, factory reset |
| HLK-LD2412 (`ld2412`) | presence + distance | about 6 m | 115200 | read information, Bluetooth, restart, factory reset |
| HLK-LD2410S (`ld2410s`) | presence + distance | about 6 m | 115200 | none (only read) |
| Seeed MR24HPC1 (`mr24hpc1`) | presence | about 4 m | 9600 | none (only read) |

`baud` in the settings is `0` for the model's own speed, or one of 9600, 19200, 38400, 57600, 115200, 230400, 256000, 460800.

## Trackers: they feed the perimeter

A tracker reports where the targets are. The node turns them into the same `tracks` of the telemetry message as before (`sensor_id` 1 to 3 is
the port, up to five tracks per port), so the server, the alarms and Studio's radar map need no change. In Studio each radar of the design
has its model (LD2450 or LD2461), which sets the field drawn on the plan, the 3D view and the radar map, and a 270° node of three LD2461 is
mounted 90° apart instead of 75°.

### LD2461

Protocol document V1.1 and specification V1.1. Big-endian frames `FF EE DD | length | command | value | checksum | DD EE FF`.

- The module reports **coordinates** (command 0x07: an int8 x and an int8 y per target, in units of 0.1 m) or **zone occupancy** (command 0x08,
  three bytes), according to its *report format* (1 coordinates, 2 zones, 3 both). The factory default is **2, zones only**, which gives no
  positions. When the TX wire is connected, the node reads the format a few seconds after start and sets 3 if it is not 1 or 3. Without the TX
  wire the panel says "reporting zones only" and the module gives no tracks until it is set from a PC tool.
- A pair of zeros is taken as an empty slot. The document gives no speed, so LD2461 tracks have speed 0.
- Zones: the module has three rectangles, each "detect only inside" (type 0) or "ignore inside" (type 1). The panel's filter maps onto that:
  *no filter* cancels the three zones, *report only what is inside* sets type 0, *ignore what is inside* sets type 1; a rectangle of zeros
  cancels that zone. Limits: ±12.7 m in 0.1 m steps.
- The serial speed conflicts inside the documents (the specification and the speed table say 9600, one sentence of the protocol says 256000).
  The default is 9600; if a real module is silent or garbled, set the speed in the panel. The node never changes the module's speed.
- The module has no configuration mode: each command is one frame answered by a frame of the same command. That the answer carries the same
  command number is deduced from the document's examples.
- Not stated in the documents: which side positive x is on beyond "x sideways, y forward", and whether a lost target is sent as (0, 0) or
  left out. The Studio "mirror" option of a radar covers the first; the second is why a (0, 0) pair is dropped.

### LD2450

Unchanged from before. The report frame is from the manufacturer's manual; the **command channel is not in the manual** (it follows the
family's public framing, `FD FC FB FA … 04 03 02 01`), so a wrong answer is shown raw in the panel. Try "Read information" first.

## Presence sensors: they become devices of the server

A presence sensor has no position. Each one is a **device** of the server with the name given in `radars[].name` (lowercase letters, digits,
`-` and `_`; it may not repeat a pin's name): the node publishes `armor/device/<node>/<name>/state` as `{"triggered":true,"distance_cm":240}`
when it changes and at least every 30 seconds, and again every time the broker reconnects. Like a mapped pin, it is registered in the server by the operator (Studio → Devices, a motion or presence
sensor whose state topic is that one); from then on it can trigger the existing alarms and automations. `distance_cm` is extra information that the server's device catalogue ignores.

The panel shows the same values live (present or not, distance, and the model's own numbers).

### LD2410B / LD2410C and LD2412

Report frames `F4 F3 F2 F1 | length | data | F8 F7 F6 F5`, little-endian. The basic and the engineering frames are both accepted. Decoded: the state
(0 nobody, 1 moving, 2 stationary, 3 both), the moving and stationary distances and energies (and on the LD2410 the detection distance). The
distance reported is the nearer of the targets that exist. The LD2412 calibration states (4 to 6) are recognised and are never reported as
presence. The energy of every distance gate (engineering frames) is not decoded. The LD2410 and LD2412 share the command framing of the
LD2450, so read firmware, restart, factory reset and Bluetooth use it.

### LD2410S

Frame `F4 F3 F2 F1 | length | state | distance (2) | reserved | F8 F7 F6 F5`; states 2 and 3 mean somebody. Reading only: its command set is
different and is not in the documents in the folder.

### MR24HPC1

Frame `53 59 | control | command | length (2) | data | checksum | 54 43`, checksum = low byte of the sum of everything before it. It reports on
change, so the node keeps what it has heard: occupied (0/1), activity (0 none, 1 motionless, 2 active), body movement (0 to 100) and proximity
(0 none, 1 approaching, 2 moving away). Because it may stay quiet for as long as nothing changes, the panel does not call a quiet MR24 "silent".
Its configuration commands are not implemented.

## What is not supported, and why

- **LD2420, LD2411, C4001**: the documents in the folder do not give a report frame that can be decoded and tested (the LD2420's command
  spreadsheet is about configuration; the LD2411 folder is an application note). Adding them would mean guessing bytes, so they are left out until
  a document with the report format or a real module is available.
- **MR24HPB1** and the generic "human presence radar" manual: a different protocol family, not needed for the perimeter.
- **Changing a module's serial speed from the panel**: built and tested for the LD2450 and LD2461, but not offered, because a wrong choice leaves
  the module unreachable from the node.
- **Sensors on the same port**: one sensor per port; the three ports are the three "radars" of the node.

## Checking a new sensor on the bench

1. Choose the model and, if needed, the speed; save and restart the node.
2. Watch the state pill: *reporting* means valid frames arrive; *bytes but no valid frame* means the wrong speed or model; *no data* means RX or power.
3. For a tracker with the TX wire: **Read information**. An answer with a version confirms the command channel; the module's raw answer is shown.
4. Walk in front of it and compare with the live values in the panel before trusting anything else.


## Who the sensor is: its firmware, its address and its Bluetooth

At start, and when a sensor is plugged in, the node asks each sensor of the command family (LD2450, LD2410B/C, LD2412, LD2410S) who it is: its **firmware version** (command `0x00A0`; the LD2410S answers `0x0000` with three words and has a serial number), and its **Bluetooth address** (`0x00A5`). A sensor that answers all zeros or all ones is "not known" and never shown as a number. The panel's *Sensors* page and the health report (`sensors` and `radar` members of `node_health`) tell for every sensor: the model, whether it is reporting, the firmware, the state of its own Bluetooth (`open`, `closed`, `none` for a model that has none, or `unknown`) and, only while it is open, the address. The sensor has no command that reads the Bluetooth switch itself: while the radio is off the sensor answers the address `08:05:04:03:02:01`, and that address is the state of the switch.

## The sensor's own Bluetooth

The LD2450, LD2410 and LD2412 have a Bluetooth radio of their own that is on from the factory and asks for no PIN: anybody near the node with the manufacturer's phone app can read the sensor and change its filters. By default the node closes it (setting `sensor_security.bluetooth_off`, on): in configuration mode it sends `0x00A4` with `0x0000`, restarts the sensor with `0x00A3` and reads the address back. It never loops: a bounded number of attempts spaced apart, then the sensor is left alone and reported (`bluetooth_alert` in the health report), and only the *Close the sensor's Bluetooth now* button of the panel allows more. The same card can turn it on on purpose for 30 minutes (for the maker's phone app); the node closes it again after that. The server raises the warning `node_sensor_bluetooth_open` while a sensor is open and the History records when it opened and when it was closed again (`sensor_bluetooth_open` and `sensor_bluetooth_closed` events on `armor/radar/<node>/events`). **Tried only with a made-up sensor, not on a board.**

## LD2412 firmware V1.24 and V1.26

The common versions of the LD2412 acknowledge commands they do not carry out: the serial speed, the engineering mode and the distance resolution (which on them breaks the link), and V1.24 does not even answer the version query. The node never changes the serial speed of any sensor, sets the resolution of an LD2412 only when its version is known to be good (1.36 or newer, which is not distributed), does not trust the answer of one that is not, and the panel and the health report say `ld2412_bad_firmware` or `ld2412_unknown_firmware`. After closing the Bluetooth the node verifies the result against the address at forty seconds because these versions are slow to restart.

## The transparent serial bridge

For the manufacturers' configuration tools for a PC, the node can hand the serial line of one sensor to an administrator for a few minutes (`POST /api/v1/bridge/open`, `/bridge/io`, `/bridge/close`, `GET /api/v1/bridge`, and the *Serial bridge* card of the panel). Only an administrator opens it, for one sensor at a time and for 1 to 30 minutes (10 by default); every exchange needs the session's random token and the administrator's login, writes at most 512 bytes and reads at most 1024, the bytes per second are limited, the speed is one of the sensors' speeds, the node's own reading of that sensor is paused while it lasts, and opening and closing are in the log and in the events. The node understands none of the bytes. `tools/serial_bridge_client.py` is the PC side: `capture` writes what the sensor sends as a timed hexadecimal dump (the first bytes of an LD2410S after power-up, for instance), `send` writes bytes and prints the answer, and `tcp` opens a port on 127.0.0.1 that a program with a "network serial port" can use as the sensor's line. The sensor's supply is **always 5 V from the node: there is no switch**, so a tool that needs the sensor to start in its boot loader needs its supply cut by hand. **Flashing a sensor from the node is not implemented**: the protocol of the manufacturers' flashing is not documented here. `tests/check_bridge_client.py` runs the client against a node made up for the test.

## The parser of the trackers

The LD2450 parser keeps the sign of the Y axis: the module can report a target a few centimetres behind itself, so the lower bound of Y is **-500 mm** (`kMinYMm`) and the range setting of the node decides what is looked at. The resolution, the origin of each target (which sensor and which track) and a short prediction that bridges a lost frame are part of the stabiliser; a full Kalman filter was judged not worth its cost, and the merge of the three sensors into one plane already existed.

## The perimeter engine

Off by default; when it is on, every rule that can raise an alarm is off or lenient until it is set. It runs on the node, next to the sensors, so a perimeter still works when the broker cannot be reached. Coordinates are millimetres on the plane of the radar calibration (x sideways, y forward).

* **Zones:** six detection zones and three exclusion zones, each a rectangle that starts at a corner and is **turned by an angle** (-180 to 180) around it, so a zone across a corner of a yard is a rectangle at 45 degrees; a name (`a-z`, digits and `_`), day and night profiles, a starting state, and for detection zones **"the target must leave"** (the zone stays occupied after the sensor loses a target inside it, until the target is seen leaving) and a **loitering time**. What is inside an exclusion zone (a window, a fan, a gate that opens) does not exist for anything else. An exclusion zone can be switched by the server like a device (`armor/device/<node>/<zone>/set`, in the words a pin takes plus `release`), so a window contact that opens can turn it off.
* **Tripwire lines:** three lines; a target crossing one gives an event with its direction (`in` or `out`) and the line counts the crossings.
* **Entry paths:** three paths of up to four zones in order; a target that moves between zones in an order no path allows, or (strict entry) appears in the middle of a path, gives a path violation.
* **Around them:** a range (-500 to 4000 mm near, 1000 to 8000 mm far), a debounce, the median of the number of targets over a few frames, still/walking/running per target, pet immunity for the presence sensors that report energy, masking detection (no target for the configured minutes while armed), a Wi-Fi jam detector, a stream mode (`live`, `change` or `events`) that lowers the traffic, and a **learning run** that suggests exclusion zones for static reflectors (nothing is applied by itself).
* **Events:** each of these is a short message on `armor/radar/<node>/events` (the `radar_event` message of ARMOR-COMMON: a fixed list of words and no free text), QoS 1, with a number for the start and a sequence number so that a repeat is told from a new one. While the broker cannot be reached the node keeps the **last 30 events in flash**, in two places written alternately with a checksum (a power cut in the middle of a write leaves the other place intact), and sends them in order when it is back.
* **Also on the node:** `GET /metrics` for administrators in the text format that Prometheus reads, the settings as a file (export and an all-or-nothing import, `POST /api/v1/config/import`, with a dry run), and the recovery of the settings store (a store that fails to open restarts the node once before anything is erased; only two failures in a row erase it).

The zone editor is in the node's own panel (*Perimeter*), in the eight languages; Studio shows the node's perimeter state and the events, and opens the panel through its proxy.

**What is not done:** Wi-Fi CSI sensing and the fusion of the sensors with it are a later phase; the presence sensors are pets-aware by energy only; and none of this has run with a sensor on a board.
