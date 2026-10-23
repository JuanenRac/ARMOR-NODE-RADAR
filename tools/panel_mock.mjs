#!/usr/bin/env node
/**
 * ARMOR-NODE-RADAR - a stand-in for a node, to work on the web panel without a board.
 * Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
 *
 *   node tools/panel_mock.mjs [port]          (default 8090)     open http://127.0.0.1:8090/
 *
 * It serves the panel from panel/ (text.js is joined in front of app.js, as tools/pack_panel.py does) and answers /api/v1 like the firmware
 * does, from memory: a fresh mock is in set-up (code TESTCODE); `--user admin:adminpass123` starts it with an administrator. It is a development
 * tool: it checks far less than the firmware, and its numbers are made up.
 */
import { createServer } from "node:http";
import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const panel = path.join(here, "..", "panel");
const port = Number(process.argv.find(a => /^\d+$/.test(a)) ?? 8090);
// --board s3-wifi: the mock plays an ESP32-S3-WROOM-1 N16R8 with no Ethernet; the default is the Waveshare ESP32-S3-ETH (s3-eth)
const board = process.argv.includes("--board") ? process.argv[process.argv.indexOf("--board") + 1] : "s3-eth";
const wired = board !== "s3-wifi";
const seeded = process.argv.includes("--user") ? process.argv[process.argv.indexOf("--user") + 1] : "";

const users = new Map();
if (seeded) { const [name, password] = seeded.split(":"); users.set(name, { password, role: "admin" }); }
const sessions = new Map();
const started = Date.now();
let rebootAt = 0;
let logText = "I (1200) armor-node: A.R.M.O.R. node armor-a1b2c3, firmware 0.2.3\nI (1500) armor-net: link up\nI (2600) armor-net: address 192.168.0.181, gateway 192.168.0.1, netmask 255.255.255.0 (wire)\nW (9000) armor-radar: radar 2: no data (RX not connected, no power or the wrong pin) (0 bytes, 0 frames, 0 bad)\n";

const config = {
  v: 1, node: { id: "armor-a1b2c3", name: "Perimeter 1" }, uplink: wired ? "ethernet" : "wifi",
  ip: { dhcp: true, address: "", netmask: "255.255.255.0", gateway: "", dns1: "", dns2: "", hostname: "" },
  ap: { enabled: true, ssid: "ARMOR", security: "wpa2", password_set: true, channel: 0, hidden: false, max_clients: 8, tx_power_dbm: 15, bandwidth_mhz: 20, country: "ES", bridge: true },
  sta: { enabled: false, ssid: "", password_set: false, backup: [] },
  mqtt: { enabled: true, uri: "mqtt://192.168.0.180:18883", username: "field-node-a1b2c3", password_set: true, heartbeat_s: 10, telemetry_ms: 200, ntp: "pool.ntp.org", backup: [] },
  radars: [{ enabled: true, model: "ld2450", baud: 0, name: "", rx: 16, tx: 15 }, { enabled: true, model: "ld2461", baud: 0, name: "", rx: 17, tx: 21 }, { enabled: true, model: "ld2410", baud: 0, name: "garage_presence", rx: 18, tx: 38 }],
  sensors: { veml7700: true, sda: 1, scl: 2, lux_fallback: -1 },
  pins: [
    { gpio: 39, name: "garden_light", mode: "output", invert: true, pull: "none", initial_on: false, safe: "off", link_timeout_s: 60, pulse_ms: 800, debounce_ms: 30, period_s: 10, freq_hz: 1000, report: "on", scale: 1, offset: 0 },
    { gpio: 40, name: "gate_contact", mode: "input", invert: false, pull: "up", initial_on: false, safe: "keep", link_timeout_s: 0, pulse_ms: 0, debounce_ms: 30, period_s: 10, freq_hz: 1000, report: "open", scale: 1, offset: 0 },
    { gpio: 2, name: "battery", mode: "adc", invert: false, pull: "none", initial_on: false, safe: "keep", link_timeout_s: 0, pulse_ms: 0, debounce_ms: 30, period_s: 10, freq_hz: 1000, report: "battery", scale: 0.0057, offset: 0 },
  ],
  ble: { mode: "setup" },
  time: { ntp_enabled: true, ntp: "pool.ntp.org", zone: "CET-1CEST,M3.5.0,M10.5.0/3" },
  system: { auto_restart_hours: 0, login_limit_per_minute: 30, reader_idle_ms: 100 },
  security: { seal_secrets: false },
  fusion: { merge_mm: 0 },
  sensor_security: { bluetooth_off: true },
  perimeter: {
    enabled: false, range_min_mm: 0, range_max_mm: 8000, debounce_frames: 2, count_window: 5, strict_entry: false, jam_detection: false, stream: "live",
    night: { enabled: false, from_min: 1320, to_min: 360 }, pet: { enabled: false, distance_cm: 200, energy: 30 }, masking: { minutes: 0, armed_only: true },
    zones: Array.from({ length: 9 }, () => ({ enabled: false, name: "", exclude: false, x_mm: 0, y_mm: 0, width_mm: 0, height_mm: 0, angle_deg: 0, profiles: 3, latch: false, loiter_s: 0, active: true })),
    lines: Array.from({ length: 3 }, () => ({ enabled: false, name: "", ax_mm: 0, ay_mm: 0, bx_mm: 0, by_mm: 0 })),
    paths: Array.from({ length: 3 }, () => ({ enabled: false, name: "", zones: [] })),
  },
  web: { mode: "both" },
  ui: { language: "en" },
};
const bridge = { active: false, sensor: 0, baud: 0, until: 0, by: "" };
const live = { garden_light: { on: false, percent: 0, value: 0, has_value: false }, gate_contact: { on: true, percent: 0, value: 0, has_value: false }, battery: { on: false, percent: 0, value: 12.61, has_value: true } };

const catalog = [];
for (let gpio = 0; gpio <= 48; ++gpio) {
  if (gpio >= 22 && gpio <= 25) continue;
  let use = "free", reason = "", header = true;
  if (gpio >= 26 && gpio <= 32) { use = "reserved"; reason = "flash"; header = false; }
  else if (gpio >= 33 && gpio <= 37) { use = "reserved"; reason = "psram"; }
  else if (gpio === 19 || gpio === 20) { use = "reserved"; reason = "usb"; }
  else if (wired && gpio >= 9 && gpio <= 14) { use = "reserved"; reason = "ethernet"; header = false; }
  else if (wired && gpio === 8) { use = "reserved"; reason = "camera"; header = false; }
  else if (wired && gpio >= 4 && gpio <= 7) use = "sd";
  else if (!wired && [43, 44, 48].includes(gpio)) use = "caution";
  else if ([0, 3, 45, 46].includes(gpio)) use = "caution";
  catalog.push({ gpio, use, reason, on_header: header, adc1: gpio >= 1 && gpio <= 10 });
}

const json = (response, code, body, headers = {}) => { response.writeHead(code, { "Content-Type": "application/json", "Cache-Control": "no-store", ...headers }); response.end(JSON.stringify(body)); };
const bodyOf = request => new Promise(resolve => { const chunks = []; request.on("data", c => chunks.push(c)); request.on("end", () => resolve(Buffer.concat(chunks))); });
const tokenOf = request => /armor_session=([0-9a-f]+)/.exec(request.headers.cookie ?? "")?.[1] ?? "";

function status() {
  return {
    node_id: config.node.id, name: config.node.name, version: "0.2.3", uptime_s: Math.floor((Date.now() - started) / 1000) + 5400, reset_reason: "power_on", heap_free: 182000, heap_min: 151000, psram_free: 7400000, partition: "ota_0",
    network: { layout: !wired ? "wifi-station+ap" : config.ap.enabled ? "ethernet+ap-bridged" : "ethernet", link_up: true, has_ip: true, ip: "192.168.0.181", netmask: "255.255.255.0", gateway: "192.168.0.1", dns: "192.168.0.1", mac: "34:85:18:a1:b2:c3", board, ethernet_available: wired, ethernet_ok: true,
      ap_active: config.ap.enabled, ap_setup: users.size === 0, ap_bridged: config.ap.bridge, ap_ssid: users.size === 0 ? "ARMOR-SETUP-A1B2C3" : config.ap.ssid, ap_channel: 6, ap_clients: 2, sta_connected: false, sta_ssid: "", sta_rssi: 0 },
    mqtt: { enabled: config.mqtt.enabled, connected: true, clock_set: true, published: 4120 + Math.floor((Date.now() - started) / 200), withheld: "" },
    web: { mode: config.web.mode, https: config.web.mode !== "http", cert_sha256: "a3f1c07d9e2b4c58a7106f3de9b2c4815d6e7f80a1b2c3d4e5f60718293a4b5c" },
    lux: 312.4, chip_temperature_c: 47.5,
    perimeter: { enabled: config.perimeter.enabled, masked: false, wifi_jam: false, buffered: 0, sent: 12, targets: 1 },
    bridge: { active: bridge.active, sensor: bridge.sensor, seconds_left: Math.max(0, Math.round((bridge.until - Date.now()) / 1000)) },
    radars: config.radars.map((r, i) => ({
      radar: i + 1, enabled: r.enabled, model: r.model, tracker: r.model === "ld2450" || r.model === "ld2461", name: r.name, rx: r.rx, tx: r.tx,
      state: !r.enabled ? "disabled" : i === 1 ? "no_data" : "reporting", state_text: "", bytes: i === 1 ? 0 : 812345, frames: i === 1 ? 0 : 27020, bad_frames: 3, fps: i === 1 ? 0 : 9.9,
      firmware: i === 0 ? "1.02.22062416" : "", firmware_known: i === 0, serial: "", mac: i === 0 ? "8F:27:2E:B8:0F:65" : "", bluetooth: i === 0 ? "open" : i === 1 ? "none" : "closed", bluetooth_alert: i === 0, bluetooth_found_open: i === 0, bluetooth_attempts: i === 0 ? 2 : 0,
      bluetooth_gave_up: i === 0, quirk: i === 2 ? "ld2412_unknown_firmware" : "", gates: i === 2 ? "applied" : "", pet_ignored: i === 2 ? 4 : 0, bridge: bridge.active && bridge.sensor === i + 1,
      tracking_mode: i === 0 ? 2 : 0, present: i === 2 ? 1 : -1, distance_cm: i === 2 ? 240 : -1,
      ...(i === 2 ? { detail: { state: 1, moving_cm: 240, moving_energy: 62, static_cm: 0, static_energy: 0, calibrating: false, detection_cm: 240 } } : {}),
    })),
  };
}

const problems = doc => {
  const list = [];
  if (doc.ap?.enabled && !String(doc.ap.ssid ?? "").trim()) list.push({ path: "ap.ssid", code: "required" });
  if (doc.ap?.enabled && doc.ap.security !== "open" && doc.ap.password !== undefined && doc.ap.password.length > 0 && doc.ap.password.length < 8) list.push({ path: "ap.password", code: "invalid_key" });
  if (doc.mqtt?.enabled && !/^mqtts?:\/\/.+/.test(doc.mqtt.uri ?? "")) list.push({ path: "mqtt.uri", code: "invalid" });
  return list;
};

const server = createServer(async (request, response) => {
  const url = new URL(request.url, "http://x");
  if (request.method === "GET" && ["/", "/index.html"].includes(url.pathname)) { response.writeHead(200, { "Content-Type": "text/html" }); return response.end(readFileSync(path.join(panel, "index.html"))); }
  if (request.method === "GET" && url.pathname === "/style.css") { response.writeHead(200, { "Content-Type": "text/css" }); return response.end(readFileSync(path.join(panel, "style.css"))); }
  if (request.method === "GET" && url.pathname === "/app.js") { response.writeHead(200, { "Content-Type": "text/javascript" }); return response.end(readFileSync(path.join(panel, "text.js"), "utf8") + "\n" + readFileSync(path.join(panel, "app.js"), "utf8")); }
  if (!url.pathname.startsWith("/api/v1/")) return json(response, 404, { error: "not_found" });

  const route = url.pathname.slice(8), method = request.method;
  const raw = await bodyOf(request);
  let body = {};
  if (raw.length && !url.pathname.endsWith("/ota")) { try { body = JSON.parse(raw.toString("utf8")); } catch { return json(response, 400, { error: "not_json" }); } }
  const session = sessions.get(tokenOf(request));
  const setup = users.size === 0;

  if (method === "GET" && route === "session") return json(response, 200, { setup, authenticated: !!session, user: session?.user ?? "", role: session?.role ?? "", node_id: config.node.id, language: config.ui.language, version: "0.2.3", setup_ssid: setup ? "ARMOR-SETUP-A1B2C3" : "", mac: "34:85:18:a1:b2:c3", board, ethernet: wired });
  if (method === "POST" && route === "setup") {
    if (!setup) return json(response, 403, { error: "forbidden" });
    if (body.code !== "TESTCODE") return json(response, 403, { error: "wrong_code" });
    if (!/^[a-z0-9_.-]{3,32}$/.test(body.user ?? "")) return json(response, 422, { error: "invalid_name" });
    if ((body.password ?? "").length < 8) return json(response, 422, { error: "weak_password" });
    if (!wired && !body.wifi_ssid) return json(response, 422, { error: "wifi_required" });
    users.set(body.user, { password: body.password, role: "admin" });
    return json(response, 200, { ok: true, restart_required: true });
  }
  if (method === "POST" && route === "login") {
    const user = users.get(body.user);
    if (!user || user.password !== body.password) return json(response, 401, { error: "wrong_credentials" });
    const token = [...Array(48)].map(() => "0123456789abcdef"[Math.floor(Math.random() * 16)]).join("");
    sessions.set(token, { user: body.user, role: user.role });
    return json(response, 200, { ok: true, restart_required: false }, { "Set-Cookie": `armor_session=${token}; Path=/; HttpOnly; SameSite=Strict` });
  }
  if (method === "POST" && route === "logout") { sessions.delete(tokenOf(request)); return json(response, 200, { ok: true }); }
  if (setup) return json(response, 403, { error: "setup_required" });
  if (!session) return json(response, 401, { error: "unauthorized" });
  if (method !== "GET" && request.headers["x-requested-with"] !== "armor") return json(response, 403, { error: "forbidden" });
  const admin = session.role === "admin";
  const needAdmin = () => { if (!admin) { json(response, 403, { error: "forbidden" }); return false; } return true; };

  if (method === "GET" && route === "status") return json(response, 200, status());
  if (method === "GET" && route === "wifi/scan") { if (!needAdmin()) return; return json(response, 200, { networks: [{ ssid: "HomeRouter", rssi: -48, channel: 6, security: "wpa2" }, { ssid: "Neighbour", rssi: -71, channel: 11, security: "wpa2wpa3" }, { ssid: "CafeOpen", rssi: -80, channel: 1, security: "open" }] }); }
  if (method === "GET" && route === "config") return json(response, 200, { config, channel_auto: 6, firmware: "0.2.3" });
  if (method === "PUT" && route === "config") {
    if (!needAdmin()) return;
    const list = problems(body);
    if (list.length) return json(response, 422, { error: "invalid", problems: list });
    for (const key of Object.keys(body)) {
      const value = body[key];
      if (value && typeof value === "object" && !Array.isArray(value) && config[key] && typeof config[key] === "object") Object.assign(config[key], value);
      else config[key] = value;
    }
    for (const section of [config.ap, config.sta, config.mqtt]) if (typeof section.password === "string" && section.password) { section.password_set = true; delete section.password; } else delete section.password;
    return json(response, 200, { ok: true, restart_required: true });
  }
  if (method === "GET" && route === "pins") return json(response, 200, { catalog, live: config.pins.filter(p => p.mode !== "disabled").map(p => ({ name: p.name, gpio: p.gpio, mode: p.mode, report: p.report, ...(live[p.name] ?? { on: false, percent: 0, value: 0, has_value: false }), fell_back: false, topic_state: `armor/device/${config.node.id}/${p.name}/state`, topic_set: "" })) });
  if (method === "POST" && route === "pins/command") {
    if (!needAdmin()) return;
    const pin = live[body.name];
    if (!pin) return json(response, 404, { error: "unknown_pin" });
    if (body.command === "on") pin.on = true; else if (body.command === "off") pin.on = false; else if (body.command === "toggle") pin.on = !pin.on; else if (body.command === "pulse") { pin.on = true; setTimeout(() => { pin.on = false; }, 800); } else return json(response, 422, { error: "invalid_command" });
    return json(response, 200, { ok: true, restart_required: false });
  }
  if (method === "GET" && route === "radars") return json(response, 200, status().radars);
  if (method === "GET" && route === "perimeter") {
    const p = config.perimeter;
    return json(response, 200, { enabled: p.enabled, armed: false, masked: false, wifi_jam: false, stream: p.stream, targets: 1, raw_targets: 1, movement: { still: 0, walking: 1, running: 0 },
      zones: p.zones.filter(z => z.enabled).map(z => ({ name: z.name, exclude: z.exclude, active: z.active, on: z.active, occupied: !z.exclude && z.name.length % 2 === 0, latched: false, occupants: 1, entries: 3, last_dwell_s: 12 })),
      lines: p.lines.filter(l => l.enabled).map(l => ({ name: l.name, ins: 4, outs: 2 })), events: { sent: 12, replayed: 0, buffered: 0, limited: 0, dropped: 0, flash_failures: 0 },
      learner: { active: false, progress: 100, frames: 1800, suggestions: [{ x_mm: -400, y_mm: 1800, width_mm: 500, height_mm: 400, frames: 1700 }] } });
  }
  if (method === "POST" && route === "perimeter/zone") { if (!needAdmin()) return; const z = config.perimeter.zones.find(x => x.enabled && x.name === body.name); if (!z) return json(response, 404, { error: "unknown_zone" }); if (body.order === "on") z.active = true; else if (body.order === "off") z.active = false; return json(response, 200, { ok: true, restart_required: false }); }
  if (method === "POST" && (route === "perimeter/armed" || route === "perimeter/learn")) { if (!needAdmin()) return; return json(response, 200, { ok: true, restart_required: false }); }
  if (method === "GET" && route === "bridge") { if (!needAdmin()) return; if (bridge.active && Date.now() > bridge.until) bridge.active = false; return json(response, 200, { active: bridge.active, sensor: bridge.sensor, baud: bridge.baud, seconds_left: Math.max(0, Math.round((bridge.until - Date.now()) / 1000)), by: bridge.by, to_sensor: 0, from_sensor: 0 }); }
  if (method === "POST" && route === "bridge/open") {
    if (!needAdmin()) return;
    if (bridge.active && Date.now() < bridge.until) return json(response, 409, { error: "bridge_busy" });
    Object.assign(bridge, { active: true, sensor: body.sensor, baud: body.baud || 256000, until: Date.now() + (body.minutes || 10) * 60000, by: session.user });
    return json(response, 200, { ok: true, token: "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf", sensor: body.sensor, baud: bridge.baud, minutes: body.minutes || 10 });
  }
  if (method === "POST" && route === "bridge/close") { if (!needAdmin()) return; if (!bridge.active) return json(response, 409, { error: "bridge_no_session" }); bridge.active = false; return json(response, 200, { ok: true, restart_required: false }); }
  if (method === "POST" && route === "config/import") {
    if (!needAdmin()) return;
    const doc = body.config ?? body;
    if (body.format !== undefined && body.format !== "armor-radar-config") return json(response, 422, { error: "invalid", problems: [{ path: "format", code: "invalid" }] });
    if (url.searchParams.get("dry") === "1") return json(response, 200, { ok: true, applied: false, restart_required: false });
    Object.assign(config, doc, { node: { ...config.node, ...doc.node, id: config.node.id } });
    return json(response, 200, { ok: true, applied: true, restart_required: true });
  }
  if (method === "POST" && route === "radars/command") {
    if (!needAdmin()) return;
    const zones = { type: 1, list: [{ x1: -500, y1: 300, x2: 500, y2: 2500 }, { x1: 0, y1: 0, x2: 0, y2: 0 }, { x1: 0, y1: 0, x2: 0, y2: 0 }] };
    if (body.radar === 2) return json(response, 200, { ok: false, error: "no_answer", firmware: "", tracking_mode: 0, last_answer: "" });
    if (body.op === "secure_bluetooth") return json(response, 200, { ok: true, error: "", firmware: "", mac: "", bluetooth: "closed", tracking_mode: 0, last_answer: "" });
    return json(response, 200, { ok: true, error: "", firmware: body.op === "read_info" ? "1.02.22062416" : "", tracking_mode: body.op === "single" ? 1 : 2, last_answer: "0000010040", ...(body.op === "read_info" ? { zones } : {}) });
  }
  if (method === "GET" && route === "users") { if (!needAdmin()) return; return json(response, 200, [...users].map(([name, u]) => ({ name, role: u.role }))); }
  if (method === "POST" && route === "users") {
    if (!needAdmin()) return;
    if (!/^[a-z0-9_.-]{3,32}$/.test(body.name ?? "")) return json(response, 422, { error: "invalid_name" });
    if ((body.password ?? "").length < 8) return json(response, 422, { error: "weak_password" });
    if (users.has(body.name)) return json(response, 409, { error: "exists" });
    users.set(body.name, { password: body.password, role: body.role === "admin" ? "admin" : "viewer" });
    return json(response, 200, { ok: true, restart_required: false });
  }
  if (method === "PUT" && route.startsWith("users/")) {
    if (!needAdmin()) return;
    const user = users.get(route.slice(6));
    if (!user) return json(response, 404, { error: "not_found" });
    if (body.role) user.role = body.role;
    if (body.password) user.password = body.password;
    return json(response, 200, { ok: true, restart_required: false });
  }
  if (method === "DELETE" && route.startsWith("users/")) {
    if (!needAdmin()) return;
    const name = route.slice(6);
    if (users.get(name)?.role === "admin" && [...users.values()].filter(u => u.role === "admin").length === 1) return json(response, 409, { error: "last_admin" });
    users.delete(name);
    return json(response, users.has(name) ? 500 : 200, { ok: true, restart_required: false });
  }
  if (method === "PUT" && route === "account") {
    const user = users.get(session.user);
    if (user.password !== body.current) return json(response, 403, { error: "wrong_password" });
    if ((body.password ?? "").length < 8) return json(response, 422, { error: "weak_password" });
    user.password = body.password;
    return json(response, 200, { ok: true, restart_required: false });
  }
  if (method === "GET" && route === "log") return json(response, 200, { next: logText.length, text: logText.slice(Number(url.searchParams.get("from") ?? 0)) });
  // the routes of the family that the panel calls and a stand-in must know too (tests/check_routes.py compares what the panel calls with this file and with the firmware)
  if (method === "GET" && route === "ble-pin") { if (!needAdmin()) return; return json(response, 200, { pin: "123456" }); }
  if (method === "GET" && route === "config/export") { if (!needAdmin()) return; return json(response, 200, { config }); }
  if (method === "GET" && route === "coredump/info") { if (!needAdmin()) return; return json(response, 200, { present: false }); }
  if (method === "POST" && route === "coredump/erase") { if (!needAdmin()) return; return json(response, 200, { ok: true }); }
  if (method === "POST" && route === "ota/copy") { if (!needAdmin()) return; return json(response, 200, { ok: true, started: true }); }
  if (method === "GET" && route === "ota/copy") { if (!needAdmin()) return; return json(response, 200, { state: "done", done: 1, total: 1, error: "", version: "0.0.3" }); }
  if (method === "POST" && route === "time") { if (!needAdmin()) return; return json(response, 200, { ok: true }); }
  if (method === "POST" && route === "ota/check") { if (!needAdmin()) return; return json(response, 200, { ok: true, started: true }); }   // the look for a release is started, then asked about
  if (method === "GET" && route === "ota/check") { if (!needAdmin()) return; return json(response, 200, { ok: true, current_version: "0.2.3", latest_version: "9.9.9", update_available: true }); }
  if (method === "POST" && route === "ota/install") { if (!needAdmin()) return; return json(response, 200, { ok: true, started: true }); }
  if (method === "GET" && route === "ota/progress") { if (!needAdmin()) return; return json(response, 200, { state: "downloading", got: 300000, total: 1200000 }); }
  if (method === "POST" && route === "ota/switch") { if (!needAdmin()) return; rebootAt = Date.now(); return json(response, 200, { ok: true, restart_required: true, slot: "ota_1", version: "0.0.0" }); }
  if (method === "POST" && route === "reboot") { if (!needAdmin()) return; rebootAt = Date.now(); return json(response, 200, { ok: true, restart_required: false }); }
  if (method === "POST" && route === "factory-reset") { if (!needAdmin()) return; if (body.confirm !== "RESET") return json(response, 422, { error: "confirm_required" }); users.clear(); return json(response, 200, { ok: true, restart_required: true }); }
  if (method === "POST" && route === "ota") { if (!needAdmin()) return; return json(response, 200, { ok: true, restart_required: true, version: "0.2.4", bytes: raw.length, sha256: "0".repeat(64) }); }
  return json(response, 404, { error: "not_found" });
});
server.listen(port, "127.0.0.1", () => console.log(`ARMOR node panel mock on http://127.0.0.1:${port}/ (${users.size ? "signed-in users: " + [...users.keys()].join(", ") : "set-up code TESTCODE"})`));
