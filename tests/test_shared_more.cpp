// ARMOR-NODE-RADAR - host tests of the rules every node shares that the other test programs only brush: the words of a health report and its optional parts, the report of a dirty start,
// the reading of the settings kept in flash, and the operations of the Bluetooth channel with a stand-in for the radio and the storage.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// Nothing here touches hardware or a network.
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if __has_include("../core/auth.hpp")
#include "../core/armor_wire.hpp"
#include "../core/auth.hpp"
#include "../core/ble_dispatch.hpp"
#include "../core/ble_frame.hpp"
#include "../core/boot_report.hpp"
#include "../core/json.hpp"
#include "../core/node_health.hpp"
#include "../core/stored_load.hpp"
#else
#include "../main/core/armor_wire.hpp"
#include "../main/core/auth.hpp"
#include "../main/core/ble_dispatch.hpp"
#include "../main/core/ble_frame.hpp"
#include "../main/core/boot_report.hpp"
#include "../main/core/json.hpp"
#include "../main/core/node_health.hpp"
#include "../main/core/stored_load.hpp"
#endif

static int failures = 0;
static int checks = 0;
#define CHECK(condition)                                                              \
  do {                                                                                \
    ++checks;                                                                         \
    if (!(condition)) {                                                               \
      ++failures;                                                                     \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);                \
    }                                                                                 \
  } while (0)

using namespace armor;

static bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

// ---- the words of a health report ----------------------------------------------------------------------------------------------------

static void test_health_words() {
  const char* expected[] = {"unknown", "power_on", "external", "software", "panic", "int_wdt", "task_wdt", "other_wdt", "deep_sleep", "brownout", "sdio", "usb", "jtag", "efuse", "power_glitch", "cpu_lockup"};
  for (int code = 1; code <= 15; ++code) CHECK(std::string(nodehealth::reset_reason_text(code)) == expected[code]);
  CHECK(std::string(nodehealth::reset_reason_text(0)) == "unknown");
  CHECK(std::string(nodehealth::reset_reason_text(99)) == "unknown");
  for (const char* abnormal : {"panic", "int_wdt", "task_wdt", "other_wdt", "brownout", "power_glitch", "cpu_lockup"}) CHECK(nodehealth::is_abnormal_reset(abnormal));
  for (const char* normal : {"power_on", "external", "software", "deep_sleep", "unknown", ""}) CHECK(!nodehealth::is_abnormal_reset(normal));
  CHECK(std::string(nodehealth::link_text(nodehealth::Link::kWifi)) == "wifi");
  CHECK(std::string(nodehealth::link_text(nodehealth::Link::kEthernet)) == "ethernet");
  CHECK(std::string(nodehealth::link_text(nodehealth::Link::kNone)) == "none");
}

static void test_link_choice() {
  using nodehealth::Link;
  nodehealth::LinkInfo cable = nodehealth::choose_link(true, true, true, false, 0);
  CHECK(cable.link == Link::kEthernet && cable.up);
  CHECK(!nodehealth::choose_link(true, true, false, false, 0).up);   // a carrier with no address is not up
  nodehealth::LinkInfo wifi = nodehealth::choose_link(false, false, true, true, -61);
  CHECK(wifi.link == Link::kWifi && wifi.up && wifi.rssi_dbm == -61);
  CHECK(nodehealth::choose_link(false, false, true, true, 5).rssi_dbm == 0);        // a positive figure is nonsense
  CHECK(nodehealth::choose_link(false, false, true, true, -200).rssi_dbm == -120);   // below the floor of the contract
  CHECK(nodehealth::choose_link(true, true, true, true, -50).link == Link::kWifi);    // the station that has joined wins over a cable
  nodehealth::LinkInfo down_cable = nodehealth::choose_link(true, false, false, false, 0);
  CHECK(down_cable.link == Link::kEthernet && !down_cable.up);
  nodehealth::LinkInfo down_wifi = nodehealth::choose_link(false, false, false, false, 0);
  CHECK(down_wifi.link == Link::kWifi && !down_wifi.up);
}

static void test_error_lines() {
  std::string text;
  CHECK(nodehealth::error_text_of_line("E (1234) wifi: auth failed", text) && text == "wifi: auth failed");
  CHECK(nodehealth::error_text_of_line("\x1b[0;31mE (77) mqtt: gone  \x1b[0m\r\n", text) && text == "mqtt: gone");
  CHECK(!nodehealth::error_text_of_line("I (1234) wifi: fine", text));
  CHECK(!nodehealth::error_text_of_line("E (1", text));
  CHECK(!nodehealth::error_text_of_line("E (123456789012345) t: late", text));   // the number of the tick is too long to be one
  CHECK(!nodehealth::error_text_of_line("E (12) ", text));
  CHECK(!nodehealth::error_text_of_line("", text));
  CHECK(nodehealth::error_text_of_line("E (5) tag: caf\xc3\xa9", text) && text == "tag: caf??");   // printable ASCII only
  const std::string long_line = "E (5) tag: " + std::string(300, 'x');
  CHECK(nodehealth::error_text_of_line(long_line, text) && text.size() == nodehealth::kMaxErrorChars);
  CHECK(nodehealth::last_error_of_log("I (1) a: ok\nE (2) b: first\nW (3) c: warn\nE (4) d: second\nI (5) e: ok") == "d: second");
  CHECK(nodehealth::last_error_of_log("E (4) d: last without a newline") == "d: last without a newline");
  CHECK(nodehealth::last_error_of_log("no errors here\n") == "");
  CHECK(nodehealth::last_error_of_log("").empty());
}

static void test_health_checks() {
  for (const char* good : {"0.8.0", "0.8.0.7", "10.20.30.40"}) CHECK(nodehealth::valid_contract(good));
  for (const char* bad : {"", "1", "1.2", "1.2.3.4.5", "1..2", ".1.2", "1.2.x", "1234567890.1.2"}) CHECK(!nodehealth::valid_contract(bad));
  for (const char* family : {"radar", "solar", "electrical", "alarm", "hmi"}) CHECK(nodehealth::valid_family(family));
  CHECK(!nodehealth::valid_family("") && !nodehealth::valid_family("server"));
  CHECK(nodehealth::topic("radar-1") == "armor/health/radar-1");
  CHECK(nodehealth::topic("not a node").empty());
}

// ---- the report of a dirty start -----------------------------------------------------------------------------------------------------

static void test_boot_causes() {
  using bootreport::Cause;
  const std::pair<Cause, const char*> names[] = {{Cause::kNone, "none"}, {Cause::kPanic, "panic"}, {Cause::kTaskWatchdog, "task_watchdog"}, {Cause::kInterruptWatchdog, "interrupt_watchdog"},
                                                 {Cause::kOtherWatchdog, "other_watchdog"}, {Cause::kBrownout, "brownout"}, {Cause::kPowerGlitch, "power_glitch"}, {Cause::kCpuLockup, "cpu_lockup"},
                                                 {Cause::kOutOfMemory, "out_of_memory"}, {Cause::kGuardMissingTask, "guard_missing_task"}, {Cause::kGuardLowMemory, "guard_low_memory"},
                                                 {Cause::kGuardDeadMan, "guard_dead_man"}, {Cause::kGuard, "guard"}};
  for (const auto& entry : names) CHECK(std::string(bootreport::cause_name(entry.first)) == entry.second);
  CHECK(bootreport::cause_of_reset("panic") == Cause::kPanic);
  CHECK(bootreport::cause_of_reset("task_wdt") == Cause::kTaskWatchdog);
  CHECK(bootreport::cause_of_reset("int_wdt") == Cause::kInterruptWatchdog);
  CHECK(bootreport::cause_of_reset("other_wdt") == Cause::kOtherWatchdog);
  CHECK(bootreport::cause_of_reset("brownout") == Cause::kBrownout);
  CHECK(bootreport::cause_of_reset("power_glitch") == Cause::kPowerGlitch);
  CHECK(bootreport::cause_of_reset("cpu_lockup") == Cause::kCpuLockup);
  for (const char* clean : {"power_on", "external", "software", "unknown"}) CHECK(bootreport::cause_of_reset(clean) == Cause::kNone);
}

static void test_boot_records() {
  using bootreport::Why;
  for (Why why : {Why::kMissingTask, Why::kLowMemory, Why::kDeadMan}) CHECK(bootreport::why_valid(bootreport::make_why(why)));
  CHECK(!bootreport::why_valid(bootreport::make_why(Why::kNone)));
  bootreport::WhyRecord broken = bootreport::make_why(Why::kLowMemory);
  broken.check ^= 1u;
  CHECK(!bootreport::why_valid(broken));
  CHECK(bootreport::guard_cause(bootreport::make_why(Why::kMissingTask), true) == bootreport::Cause::kGuardMissingTask);
  CHECK(bootreport::guard_cause(bootreport::make_why(Why::kLowMemory), true) == bootreport::Cause::kGuardLowMemory);
  CHECK(bootreport::guard_cause(bootreport::make_why(Why::kDeadMan), true) == bootreport::Cause::kGuardDeadMan);
  CHECK(bootreport::guard_cause(bootreport::make_why(Why::kDeadMan), false) == bootreport::Cause::kGuard);   // the memory did not survive
  CHECK(bootreport::guard_cause(broken, true) == bootreport::Cause::kGuard);
  const bootreport::CrashRecord crash = bootreport::make_crash(500, 30000, 90000);
  CHECK(bootreport::crash_valid(crash) && crash.internal_largest == 30000);   // never more than what was free
  bootreport::CrashRecord torn = crash;
  torn.uptime_s += 1;
  CHECK(!bootreport::crash_valid(torn));
}

static void test_boot_classification() {
  using bootreport::Cause;
  bootreport::Facts facts;
  facts.reset_reason = "power_on";
  bootreport::Report clean = bootreport::classify(facts);
  CHECK(!clean.dirty && clean.cause == Cause::kNone && !bootreport::should_report(clean, 10));

  facts.reset_reason = "task_wdt";
  facts.coredump_present = true;
  bootreport::Report plain = bootreport::classify(facts);
  CHECK(plain.dirty && plain.cause == Cause::kTaskWatchdog && plain.coredump && !plain.has_uptime_before && !plain.has_heap);
  CHECK(bootreport::should_report(plain, 599) && !bootreport::should_report(plain, bootreport::kReportForS));

  facts.kept_memory = true;
  facts.crash = bootreport::make_crash(1200, 41000, 20000);
  bootreport::Report crashed = bootreport::classify(facts);
  CHECK(crashed.has_uptime_before && crashed.uptime_before_s == 1200 && crashed.has_heap && crashed.heap_internal_free == 41000 && crashed.heap_internal_largest == 20000);

  facts.oom = nodeguard::make_oom(9000, 20000, 77, 5);   // the largest block is cut to what was free
  bootreport::Report oom = bootreport::classify(facts);
  CHECK(oom.cause == Cause::kOutOfMemory && oom.uptime_before_s == 77 && oom.heap_internal_free == 9000 && oom.heap_internal_largest == 9000);

  bootreport::Facts guard;
  guard.reset_reason = "software";
  guard.by_guard = true;
  guard.deliberate = true;
  bootreport::Report guarded = bootreport::classify(guard);
  CHECK(guarded.dirty && guarded.cause == Cause::kGuard && guarded.guard && guarded.deliberate);
  guard.kept_memory = true;
  guard.why = bootreport::make_why(bootreport::Why::kDeadMan);
  CHECK(bootreport::classify(guard).cause == Cause::kGuardDeadMan);
}

static void test_boot_json() {
  bootreport::Facts facts;
  facts.reset_reason = "panic";
  facts.kept_memory = true;
  facts.crash = bootreport::make_crash(60, 50000, 40000);
  facts.coredump_present = true;
  json::Writer w;
  bootreport::write_json(w, bootreport::classify(facts));
  const std::string full = w.str();
  CHECK(contains(full, "\"cause\":\"panic\"") && contains(full, "\"uptime_before_s\":60") && contains(full, "\"heap_internal_free\":50000") && contains(full, "\"coredump\":true"));
  bootreport::Facts bare;
  bare.reset_reason = "brownout";
  json::Writer v;
  bootreport::write_json(v, bootreport::classify(bare));
  CHECK(!contains(v.str(), "uptime_before_s") && !contains(v.str(), "heap_internal"));

  bootreport::GateInfo gate;
  gate.closed = true;
  gate.transitions = 3;
  json::Writer g;
  bootreport::write_gate_json(g, gate);
  CHECK(contains(g.str(), "\"closed\":true") && contains(g.str(), "\"transitions\":3") && contains(g.str(), "close_free") && contains(g.str(), "open_block"));

  bootreport::ConfigCheck check;
  CHECK(!check.any());
  json::Writer none;
  bootreport::write_config_check_json(none, check);
  CHECK(none.str() == "{}");
  check.snapshot = "restored";
  check.reverted = "failed";
  CHECK(check.any());
  json::Writer both;
  bootreport::write_config_check_json(both, check);
  CHECK(contains(both.str(), "\"snapshot\":\"restored\"") && contains(both.str(), "\"reverted\":\"failed\""));
}

// A health report with every optional part says what the contract allows, and the generated reader of the contract accepts it and writes it back as it is.
static void test_full_health_report() {
  nodehealth::Report report;
  report.node_id = "radar-1";
  report.family = "radar";
  report.firmware = "0.8.0.1";
  report.board = "s3-eth";
  report.contract = "0.8.1";
  report.timestamp_ms = 1700000000000ull;
  report.uptime_s = 300;
  report.reset_reason = "panic";
  report.heap_free = 200000;
  report.heap_min = 300000;   // never above what is free
  report.has_memory = true;
  report.memory.internal_free = 90000;
  report.memory.internal_min = 120000;
  report.memory.internal_largest = 100000;
  report.memory.psram_free = 7000000;
  report.memory.psram_total = 8000000;
  report.link = nodehealth::choose_link(false, false, true, true, -58);
  report.broker_reconnects = 4;
  report.low_stack_tasks = 99;
  report.last_error = "mqtt: gone";
  report.has_control = true;
  report.control_state = "paused";
  report.control_since_ms = 1699999999000ull;
  report.control_by = "ana";
  report.control_until_s = 90000;
  report.maintenance = true;
  report.maintenance_until_s = 120;
  bootreport::Facts facts;
  facts.reset_reason = "panic";
  facts.kept_memory = true;
  facts.crash = bootreport::make_crash(60, 50000, 40000);
  report.has_boot = true;
  report.boot = bootreport::classify(facts);
  report.has_gate = true;
  report.gate.closed = true;
  report.has_coredump = true;
  report.coredump_present = true;
  report.coredump_size = 65536;
  report.has_config_check = true;
  report.config_check.snapshot = "restored";
  const std::string text = nodehealth::to_json(report);
  CHECK(contains(text, "\"heap_min\":200000"));
  CHECK(contains(text, "\"low_stack_tasks\":64"));
  CHECK(contains(text, "\"control_until_s\":86400"));
  CHECK(contains(text, "\"psram_free\":7000000"));
  json::Value value;
  CHECK(json::parse(text, value));
  CHECK(wire::validate_message("node_health", value));
  std::string again;
  CHECK(wire::rewrite_message("node_health", value, again));
  json::Value reread;
  std::string second;
  CHECK(json::parse(again, reread) && wire::validate_message("node_health", reread) && wire::rewrite_message("node_health", reread, second) && second == again);

  nodehealth::Report minimal;
  minimal.node_id = "alarm-1";
  minimal.family = "alarm";
  minimal.firmware = "0.3.6";
  minimal.contract = "not a version";
  minimal.link.link = nodehealth::Link::kNone;
  minimal.has_control = true;   // running: no since, no by, no time left
  const std::string small = nodehealth::to_json(minimal);
  CHECK(!contains(small, "contract") && !contains(small, "control_since_ms") && contains(small, "\"link_up\":false"));
  json::Value small_value;
  CHECK(json::parse(small, small_value) && wire::validate_message("node_health", small_value));
}

// ---- the settings kept in flash ---------------------------------------------------------------------------------------------------------

namespace armor::config {
struct ToyProblem {
  std::string path;
  std::string what;
};
struct ToySettings {
  long long baud = 9600;
  std::vector<long long> ports{1, 2, 3};
};
inline void read_settings(const json::Value& document, ToySettings& out, std::vector<ToyProblem>& problems) {
  if (const json::Value* baud = document.get("baud")) {
    if (!baud->is_number()) problems.push_back({"baud", "type"});
    else out.baud = static_cast<long long>(baud->number);
  }
  if (const json::Value* ports = document.get("ports")) {
    out.ports.clear();
    for (std::size_t i = 0; i < ports->items.size(); ++i) {
      if (!ports->items[i].is_number()) problems.push_back({"ports." + std::to_string(i), "type"});
      else out.ports.push_back(static_cast<long long>(ports->items[i].number));
    }
  }
}
inline std::vector<ToyProblem> validate(const ToySettings& settings) {
  std::vector<ToyProblem> problems;
  if (settings.baud < 1200 || settings.baud > 115200) problems.push_back({"baud", "range"});
  if (settings.ports.size() > 4) problems.push_back({"ports", "count"});
  return problems;
}
}  // namespace armor::config

static void test_stored_settings() {
  using armor::config::ToyProblem;
  using armor::config::ToySettings;
  const ToySettings base;
  ToySettings out;
  std::vector<ToyProblem> dropped;
  CHECK(config::load_stored("{\"baud\":19200,\"unknown\":1}", base, out, dropped) && out.baud == 19200 && dropped.empty());
  CHECK(config::load_stored("{\"baud\":\"fast\",\"ports\":[5,6]}", base, out, dropped) && out.baud == 9600 && out.ports.size() == 2 && dropped.size() == 1 && dropped[0].path == "baud");   // one field is left out, the rest is kept
  CHECK(config::load_stored("{\"ports\":[5,\"x\",7]}", base, out, dropped) && out.ports.size() == 2 && out.ports[1] == 7);   // an item of a list is dropped by its place
  CHECK(config::load_stored("{\"baud\":999999}", base, out, dropped) && out.baud == 9600 && dropped.size() == 1);   // out of range: the first value of that one thing
  CHECK(config::load_stored("{\"ports\":[1,2,3,4,5,6]}", base, out, dropped) && out.ports == base.ports && dropped.size() == 1);   // a problem about a whole list drops the list
  CHECK(!config::load_stored("not json", base, out, dropped) && dropped.size() == 1 && dropped[0].what == "not_json");
  json::Value document;
  CHECK(json::parse("{\"a\":{\"b\":[1,2]},\"c\":1}", document));
  CHECK(config::detail::drop_path(document, "a.b.1") && document.get("a")->get("b")->items.size() == 1);
  CHECK(config::detail::drop_path(document, "a.zzz.0") && document.get("a") == nullptr);   // the place is not there: the enclosing object goes instead
  CHECK(!config::detail::drop_path(document, "nothing.at.all"));
  CHECK(!config::detail::drop_path(document, ""));
}

// ---- the Bluetooth channel --------------------------------------------------------------------------------------------------------------

struct FakeBackend : ble::Backend {
  bool users = false;
  bool code_ok = true;
  auth::Result add_result = auth::Result::kOk;
  bool password_ok = true;
  auth::Role role = auth::Role::kAdmin;
  int put_code = 0;
  bool scan_ok = true;
  std::string scan_error;
  bool restarted = false;
  std::uint64_t clock = 1000;
  bool has_users() override { return users; }
  std::string hello_json() override { return "{\"node\":\"x\"}"; }
  bool setup_code_ok(std::string_view) override { return code_ok; }
  auth::Result add_administrator(std::string_view, std::string_view) override { if (add_result == auth::Result::kOk) users = true; return add_result; }
  bool verify(std::string_view, std::string_view, auth::Role& out) override { out = role; return password_ok; }
  std::string config_json() override { return "{\"config\":{}}"; }
  int config_put(std::string_view document, std::string& data) override { data = put_code == 0 ? "" : "{\"problems\":[]}"; last_document = std::string(document); return put_code; }
  std::string status_json() override { return "{\"up\":true}"; }
  bool wifi_scan(std::string& data, std::string& error) override { data = "{\"networks\":[]}"; error = scan_error; return scan_ok; }
  void restart_soon() override { restarted = true; }
  std::uint64_t now_ms() override { return clock; }
  std::string last_document;
};

static std::string ask(FakeBackend& backend, ble::Session& session, auth::LoginThrottle& throttle, const std::string& text) {
  ble::Request request;
  CHECK(ble::parse_request(text, request));
  return ble::handle(backend, session, throttle, request);
}

static void test_ble_operations() {
  FakeBackend backend;
  ble::Session session;
  auth::LoginThrottle throttle;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":1,\"op\":\"hello\"}"), "\"node\":\"x\""));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":2,\"op\":\"login\",\"args\":{}}"), "setup_required"));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":2,\"op\":\"status\"}"), "setup_required"));

  backend.code_ok = false;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":3,\"op\":\"setup\",\"args\":{\"code\":\"bad\"}}"), "wrong_code"));
  backend.code_ok = true;
  backend.add_result = auth::Result::kInvalidName;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":4,\"op\":\"setup\",\"args\":{}}"), "invalid_name"));
  backend.add_result = auth::Result::kWeakPassword;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":4,\"op\":\"setup\",\"args\":{}}"), "weak_password"));
  backend.add_result = auth::Result::kStorage;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":4,\"op\":\"setup\",\"args\":{}}"), "storage"));
  backend.add_result = auth::Result::kOk;
  const std::string made = ask(backend, session, throttle, "{\"id\":5,\"op\":\"setup\",\"args\":{\"code\":\"ok\",\"user\":\"ana\",\"password\":\"long enough password\"}}");
  CHECK(contains(made, "restart_required") && session.authenticated && session.role == auth::Role::kAdmin && session.user == "ana");
  CHECK(contains(ask(backend, session, throttle, "{\"id\":6,\"op\":\"setup\",\"args\":{}}"), "forbidden"));   // there is a user now

  CHECK(contains(ask(backend, session, throttle, "{\"id\":7,\"op\":\"config.get\"}"), "\"config\""));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":8,\"op\":\"status\"}"), "\"up\":true"));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":9,\"op\":\"config.put\",\"args\":{}}"), "invalid"));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":9,\"op\":\"config.put\",\"args\":{\"config\":5}}"), "invalid"));
  const std::string put = ask(backend, session, throttle, "{\"id\":10,\"op\":\"config.put\",\"args\":{\"config\":{\"a\":[1,true,null,\"t\",{\"b\":2.5}]}}}");
  CHECK(contains(put, "\"ok\":true") && contains(backend.last_document, "\"a\":[1,true,null,\"t\",{\"b\":2.5}]"));
  backend.put_code = 422;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":11,\"op\":\"config.put\",\"args\":{\"config\":{}}}"), "invalid"));
  backend.put_code = 500;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":11,\"op\":\"config.put\",\"args\":{\"config\":{}}}"), "storage"));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":12,\"op\":\"wifi.scan\"}"), "networks"));
  backend.scan_ok = false;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":13,\"op\":\"wifi.scan\"}"), "scan_failed"));
  backend.scan_error = "radio_busy";
  CHECK(contains(ask(backend, session, throttle, "{\"id\":13,\"op\":\"wifi.scan\"}"), "radio_busy"));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":14,\"op\":\"nothing\"}"), "unknown_op"));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":15,\"op\":\"reboot\"}"), "\"ok\":true") && backend.restarted);
  CHECK(contains(ask(backend, session, throttle, "{\"id\":16,\"op\":\"logout\"}"), "\"ok\":true") && !session.authenticated);
  CHECK(contains(ask(backend, session, throttle, "{\"id\":17,\"op\":\"status\"}"), "unauthorized"));
}

static void test_ble_roles_and_throttle() {
  FakeBackend backend;
  backend.users = true;
  backend.role = auth::Role::kViewer;
  ble::Session session;
  session.peer = 0xA1B2C3D4u;
  auth::LoginThrottle throttle;
  CHECK(contains(ask(backend, session, throttle, "{\"id\":1,\"op\":\"login\",\"args\":{\"user\":\"v\",\"password\":\"p\"}}"), "\"role\":\"viewer\""));
  for (const char* op : {"config.put", "wifi.scan", "reboot"}) CHECK(contains(ask(backend, session, throttle, std::string("{\"id\":2,\"op\":\"") + op + "\",\"args\":{\"config\":{}}}"), "forbidden"));
  CHECK(contains(ask(backend, session, throttle, "{\"id\":3,\"op\":\"status\"}"), "\"up\":true"));   // a viewer may look

  backend.password_ok = false;
  ble::Session phone;
  phone.peer = 0x01020304u;
  for (int attempt = 0; attempt < 5; ++attempt) CHECK(contains(ask(backend, phone, throttle, "{\"id\":4,\"op\":\"login\",\"args\":{\"user\":\"v\",\"password\":\"x\"}}"), "wrong_credentials"));
  const std::string locked = ask(backend, phone, throttle, "{\"id\":5,\"op\":\"login\",\"args\":{\"user\":\"v\",\"password\":\"x\"}}");
  CHECK(contains(locked, "too_many_attempts") && contains(locked, "wait_s"));
  backend.password_ok = true;
  ble::Session owner;
  owner.peer = 0x0A0B0C0Du;   // another phone is not blocked by the first one
  CHECK(contains(ask(backend, owner, throttle, "{\"id\":6,\"op\":\"login\",\"args\":{\"user\":\"v\",\"password\":\"p\"}}"), "\"ok\":true"));
  backend.clock += 10 * 60 * 1000;   // and after a while the first one may try again
  CHECK(contains(ask(backend, phone, throttle, "{\"id\":7,\"op\":\"login\",\"args\":{\"user\":\"v\",\"password\":\"p\"}}"), "\"ok\":true"));

  ble::Session unknown;   // a phone that is not known counts on the shared throttle alone, five at a time
  auth::LoginThrottle shared;
  backend.password_ok = false;
  for (int attempt = 0; attempt < 5; ++attempt) ask(backend, unknown, shared, "{\"id\":8,\"op\":\"login\",\"args\":{}}");
  CHECK(contains(ask(backend, unknown, shared, "{\"id\":9,\"op\":\"login\",\"args\":{}}"), "too_many_attempts"));
  ble::Session setup_session;
  FakeBackend fresh;
  fresh.code_ok = false;
  auth::LoginThrottle setup_throttle;
  for (int attempt = 0; attempt < 5; ++attempt) ask(fresh, setup_session, setup_throttle, "{\"id\":10,\"op\":\"setup\",\"args\":{}}");
  CHECK(contains(ask(fresh, setup_session, setup_throttle, "{\"id\":11,\"op\":\"setup\",\"args\":{}}"), "too_many_attempts"));
}

int main() {
  test_health_words();
  test_link_choice();
  test_error_lines();
  test_health_checks();
  test_boot_causes();
  test_boot_records();
  test_boot_classification();
  test_boot_json();
  test_full_health_report();
  test_stored_settings();
  test_ble_operations();
  test_ble_roles_and_throttle();
  std::printf("shared more: %d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
