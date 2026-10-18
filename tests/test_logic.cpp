// ARMOR-RADAR - host tests of the small decisions the other tests only reach through the settings: the framer that finds radar frames in a byte stream, the names and report fields of
// mapped pins, the broker password that belongs to its broker, and the limits of the climate and light checks.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// No serial line, pin or broker is used: the bytes and the settings documents are written out in the tests.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../main/core/climate.hpp"
#include "../main/core/frame_framer.hpp"
#include "../main/core/gpio_logic.hpp"
#include "../main/core/json.hpp"
#include "../main/core/node_config.hpp"

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

static config::Problems read(config::Settings& s, const std::string& text) {
  json::Value document;
  const bool ok = json::parse(text, document);
  CHECK(ok);
  config::Problems problems;
  config::read_settings(document, s, problems);
  return problems;
}
static bool has(const config::Problems& problems, const std::string& path, const std::string& code) {
  for (const config::Problem& p : problems) if (p.path == path && p.code == code) return true;
  return false;
}

// ---- the framer ------------------------------------------------------------------------------------------------------------------------

static ProtocolSpec spec() {
  ProtocolSpec s;
  s.header = {0xAA, 0xFF, 0, 0};
  s.header_length = 2;
  s.tail = {0x55, 0xCC, 0, 0};
  s.tail_length = 2;
  s.frame_length = 8;   // header, four payload bytes, tail
  return s;
}
static std::vector<std::uint8_t> frame(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d) { return {0xAA, 0xFF, a, b, c, d, 0x55, 0xCC}; }

static void test_spec() {
  CHECK(spec().configured());
  CHECK(!ProtocolSpec::unconfigured().configured());
  ProtocolSpec s = spec();
  s.header_length = 0; CHECK(!s.configured());
  s = spec(); s.header_length = 5; CHECK(!s.configured());                 // longer than a marker can be
  s = spec(); s.tail_length = 5; CHECK(!s.configured());
  s = spec(); s.frame_length = 4; CHECK(!s.configured());                  // no room for a payload
  s = spec(); s.frame_length = 5; CHECK(s.configured());                   // one byte of payload
  s = spec(); s.frame_length = kMaxFrameLength; CHECK(s.configured());
  s = spec(); s.frame_length = kMaxFrameLength + 1; CHECK(!s.configured());
  s = spec(); s.tail_length = 0; s.frame_length = 6; CHECK(s.configured());   // a frame with no tail is allowed
}

static void test_framer() {
  std::vector<std::vector<std::uint8_t>> got;
  const FrameFramer::FrameHandler keep = [&](const std::uint8_t* data, std::size_t length) { got.emplace_back(data, data + length); };
  {
    FrameFramer framer(spec());
    CHECK(framer.configured());
    const std::vector<std::uint8_t> one = frame(1, 2, 3, 4);
    framer.feed(one.data(), one.size(), keep);
    CHECK(got.size() == 1 && got[0] == one && framer.stats().frames == 1 && framer.stats().dropped_bytes == 0 && framer.stats().bad_frames == 0);
  }
  got.clear();
  {   // a frame that arrives a byte at a time, and two that arrive together
    FrameFramer framer(spec());
    const std::vector<std::uint8_t> a = frame(1, 2, 3, 4), b = frame(5, 6, 7, 8);
    for (const std::uint8_t byte : a) framer.feed(&byte, 1, keep);
    CHECK(got.size() == 1 && got[0] == a);
    std::vector<std::uint8_t> both = a;
    both.insert(both.end(), b.begin(), b.end());
    framer.feed(both.data(), both.size(), keep);
    CHECK(got.size() == 3 && got[1] == a && got[2] == b && framer.stats().frames == 3);
  }
  got.clear();
  {   // a stream that starts in the middle of a frame, with noise between frames
    FrameFramer framer(spec());
    std::vector<std::uint8_t> stream = {0x01, 0x02, 0x55, 0xCC, 0xAA};   // the end of a frame that was cut, and half a header
    const std::vector<std::uint8_t> a = frame(9, 9, 9, 9);
    stream.insert(stream.end(), a.begin(), a.end());
    stream.push_back(0x00);
    stream.push_back(0xAA);
    const std::vector<std::uint8_t> b = frame(8, 8, 8, 8);
    stream.insert(stream.end(), b.begin(), b.end());
    framer.feed(stream.data(), stream.size(), keep);
    CHECK(got.size() == 2 && got[0] == a && got[1] == b);
    CHECK(framer.stats().frames == 2 && framer.stats().dropped_bytes >= 5);
  }
  got.clear();
  {   // a false header: the bytes look like a header but the tail is not there, and a real frame that starts inside them is still found
    FrameFramer framer(spec());
    std::vector<std::uint8_t> stream = {0xAA, 0xFF, 0xAA, 0xFF, 1, 2, 3, 4, 0x55, 0xCC};   // the first AA FF is a false header; the frame begins at the second
    framer.feed(stream.data(), stream.size(), keep);
    CHECK(got.size() == 1 && got[0] == frame(1, 2, 3, 4) && framer.stats().bad_frames >= 1);
  }
  got.clear();
  {   // a frame whose tail is wrong is never handed on
    FrameFramer framer(spec());
    std::vector<std::uint8_t> broken = frame(1, 2, 3, 4);
    broken[7] = 0xCD;
    framer.feed(broken.data(), broken.size(), keep);
    CHECK(got.empty() && framer.stats().frames == 0 && framer.stats().bad_frames >= 1);
    const std::vector<std::uint8_t> good = frame(4, 3, 2, 1);
    framer.feed(good.data(), good.size(), keep);
    CHECK(got.size() == 1 && got[0] == good);   // and the stream goes on
  }
  got.clear();
  {   // an unconfigured spec emits nothing, whatever it is fed: no data can be invented
    FrameFramer framer(ProtocolSpec::unconfigured());
    CHECK(!framer.configured());
    const std::vector<std::uint8_t> one = frame(1, 2, 3, 4);
    framer.feed(one.data(), one.size(), keep);
    CHECK(got.empty() && framer.stats().frames == 0 && framer.stats().dropped_bytes == 0);
  }
  got.clear();
  {   // a long run of noise and of header bytes never overruns the buffer
    FrameFramer framer(spec());
    std::vector<std::uint8_t> noise(5000, 0xAA);
    framer.feed(noise.data(), noise.size(), keep);
    std::vector<std::uint8_t> more(5000, 0x11);
    framer.feed(more.data(), more.size(), keep);
    const std::vector<std::uint8_t> one = frame(1, 2, 3, 4);
    framer.feed(one.data(), one.size(), keep);
    CHECK(got.size() == 1 && got[0] == one);
  }
  got.clear();
  {   // the longest frame the framer holds
    ProtocolSpec big = spec();
    big.frame_length = kMaxFrameLength;
    FrameFramer framer(big);
    std::vector<std::uint8_t> long_frame(kMaxFrameLength, 0x42);
    long_frame[0] = 0xAA; long_frame[1] = 0xFF; long_frame[kMaxFrameLength - 2] = 0x55; long_frame[kMaxFrameLength - 1] = 0xCC;
    framer.feed(long_frame.data(), long_frame.size(), keep);
    framer.feed(long_frame.data(), long_frame.size(), keep);
    CHECK(got.size() == 2 && got[1] == long_frame);
  }
  got.clear();
  {   // a frame with no tail is complete as soon as its length is reached
    ProtocolSpec headless_tail = spec();
    headless_tail.tail_length = 0;
    headless_tail.frame_length = 6;
    FrameFramer framer(headless_tail);
    const std::vector<std::uint8_t> bytes = {0xAA, 0xFF, 1, 2, 3, 4};
    framer.feed(bytes.data(), bytes.size(), keep);
    CHECK(got.size() == 1 && got[0] == bytes);
  }
}

// ---- the names and report fields of mapped pins ---------------------------------------------------------------------------------------------------

static void test_slugs_and_reports() {
  using config::PinMode;
  for (const char* ok : {"a", "garden_light", "pin_01", "0", "_", "x_"}) CHECK(config::valid_slug(ok));
  CHECK(config::valid_slug(std::string(24, 'a')));
  for (const char* bad : {"", "Garden", "a-b", "a b", "a/b", "a.b", "a+", "a#", "\xC3\xA9"}) CHECK(!config::valid_slug(bad));
  CHECK(!config::valid_slug(std::string(25, 'a')));
  CHECK(std::string(config::default_report(PinMode::kInput)) == "triggered" && std::string(config::default_report(PinMode::kOutput)) == "on" && std::string(config::default_report(PinMode::kPwm)) == "brightness" &&
        std::string(config::default_report(PinMode::kAdc)) == "mv" && std::string(config::default_report(PinMode::kDisabled)).empty());
  // every mode accepts its own default and the fields its kind may report, and nothing else
  for (const PinMode mode : {PinMode::kInput, PinMode::kOutput, PinMode::kPwm, PinMode::kAdc}) CHECK(config::report_allowed(mode, config::default_report(mode)));
  CHECK(config::report_allowed(PinMode::kInput, "open") && config::report_allowed(PinMode::kInput, "tamper") && !config::report_allowed(PinMode::kInput, "brightness") && !config::report_allowed(PinMode::kInput, "mv"));
  CHECK(config::report_allowed(PinMode::kOutput, "locked") && !config::report_allowed(PinMode::kOutput, "triggered") && !config::report_allowed(PinMode::kOutput, "open"));
  CHECK(config::report_allowed(PinMode::kPwm, "brightness") && !config::report_allowed(PinMode::kPwm, "on"));
  for (const char* field : {"mv", "battery", "brightness", "humidity", "temperature", "lux", "power_w", "co_ppm"}) CHECK(config::report_allowed(PinMode::kAdc, field));
  CHECK(!config::report_allowed(PinMode::kAdc, "on") && !config::report_allowed(PinMode::kAdc, "") && !config::report_allowed(PinMode::kAdc, "MV"));
  for (const char* field : {"on", "triggered", "mv", ""}) CHECK(!config::report_allowed(PinMode::kDisabled, field));
  config::MappedPin pin;
  pin.mode = PinMode::kInput;
  CHECK(config::report_of(pin) == "triggered");
  pin.report = "tamper";
  CHECK(config::report_of(pin) == "tamper");
  // the settings refuse a field the mode cannot report, a duplicate name and a name that cannot be a topic
  config::Settings s = config::default_settings("a1b2c3");
  s.uplink = config::Uplink::kWifi; s.sta.enabled = true; s.sta.ssid = "casa";
  for (config::RadarLine& radar : s.radars) { radar.enabled = false; }
  config::MappedPin input;
  input.gpio = 5; input.name = "door"; input.mode = PinMode::kInput;
  s.pins = {input};
  CHECK(config::validate(s).empty());
  s.pins[0].report = "brightness"; CHECK(has(config::validate(s), "pins.0.report", "invalid")); s.pins[0].report = "";
  s.pins[0].name = "Door"; CHECK(has(config::validate(s), "pins.0.name", "invalid")); s.pins[0].name = "";
  CHECK(has(config::validate(s), "pins.0.name", "required")); s.pins[0].name = "door";
  config::MappedPin second = input;
  second.gpio = 6;
  s.pins.push_back(second);
  CHECK(has(config::validate(s), "pins.1.name", "conflict")); s.pins[1].name = "window"; CHECK(config::validate(s).empty());
  s.pins[1].gpio = 5; CHECK(has(config::validate(s), "pins.1.gpio", "conflict")); s.pins[1].gpio = 33; CHECK(has(config::validate(s), "pins.1.gpio", "reserved"));
  s.pins[1].gpio = -1; CHECK(has(config::validate(s), "pins.1.gpio", "required"));
  s.pins[1].mode = PinMode::kDisabled; CHECK(config::validate(s).empty());   // a pin that is off is not looked at
  s.pins[1] = second; s.pins[1].name = "window"; s.pins[1].mode = PinMode::kAdc; s.pins[1].gpio = 15; CHECK(has(config::validate(s), "pins.1.mode", "no_adc")); s.pins[1].gpio = 6; CHECK(config::validate(s).empty());
  // a pin that is not in the document's list is dropped, and more than the limit is refused whole
  std::string many = "{\"pins\":[";
  for (int i = 0; i < 17; ++i) many += std::string(i ? "," : "") + "{\"gpio\":1,\"name\":\"p" + std::to_string(i) + "\",\"mode\":\"input\"}";
  config::Settings t = s;
  CHECK(has(read(t, many + "]}"), "pins", "too_many") && t.pins.size() == s.pins.size());
  CHECK(has(read(t, "{\"pins\":{}}"), "pins", "invalid"));
  CHECK(has(read(t, "{\"pins\":[{\"gpio\":1,\"mode\":\"analog\"}]}"), "pins.0.mode", "invalid"));
  CHECK(has(read(t, "{\"pins\":[{\"gpio\":1,\"pull\":\"middle\",\"safe\":\"maybe\"}]}"), "pins.0.pull", "invalid"));
  CHECK(has(read(t, "{\"pins\":[{\"gpio\":99}]}"), "pins.0.gpio", "range"));
  CHECK(has(read(t, "{\"pins\":[{\"gpio\":1,\"scale\":1e9,\"offset\":\"x\"}]}"), "pins.0.scale", "range"));
  // eight PWM pins at most, on at most four different frequencies
  config::Settings pwm = s;
  pwm.pins.clear();
  for (int i = 0; i < 9; ++i) { config::MappedPin p; p.gpio = 1 + i; p.name = "pwm" + std::to_string(i); p.mode = PinMode::kPwm; pwm.pins.push_back(p); }
  CHECK(has(config::validate(pwm), "pins", "too_many_pwm"));
  pwm.pins.pop_back();
  CHECK(!has(config::validate(pwm), "pins", "too_many_pwm"));
  for (int i = 0; i < 5; ++i) pwm.pins[static_cast<std::size_t>(i)].freq_hz = 1000 * (i + 1);
  CHECK(has(config::validate(pwm), "pins", "too_many_pwm_freq"));
  pwm.pins[4].freq_hz = 1000;
  CHECK(!has(config::validate(pwm), "pins", "too_many_pwm_freq"));
}

// ---- the broker password belongs to its broker ----------------------------------------------------------------------------------------------------

static void test_broker_host() {
  CHECK(config::detail::broker_host("mqtt://Broker.Local:1883/path") == "broker.local" && config::detail::broker_host("mqtts://10.0.0.1") == "10.0.0.1" && config::detail::broker_host("broker") == "broker" &&
        config::detail::broker_host("").empty() && config::detail::broker_host("mqtt://").empty() && config::detail::broker_host("mqtt://h/x:y") == "h");
}

static void test_password_follows_its_broker() {
  config::Settings s = config::default_settings("a1b2c3");
  s.mqtt.enabled = true; s.mqtt.uri = "mqtt://broker.local:1883"; s.mqtt.username = "node"; s.mqtt.password = "secret-pass";
  // the same host (a different port, other letter case, a path) keeps the password; the panel never holds it
  config::Settings t = s;
  CHECK(read(t, "{\"mqtt\":{\"uri\":\"mqtt://BROKER.local:1884\"}}").empty() && t.mqtt.password == "secret-pass");
  CHECK(read(t, "{\"mqtt\":{\"uri\":\"mqtts://broker.local:8883/x\"}}").empty() && t.mqtt.password == "secret-pass");
  // another host: the stored password is not sent there
  t = s;
  CHECK(read(t, "{\"mqtt\":{\"uri\":\"mqtt://elsewhere.example:1883\"}}").empty() && t.mqtt.password.empty());
  // with a new password typed in the same save, the new one is the password of the new host
  t = s;
  CHECK(read(t, "{\"mqtt\":{\"uri\":\"mqtt://elsewhere.example\",\"password\":\"typed-pass\"}}").empty() && t.mqtt.password == "typed-pass");
  // a document that does not name a uri leaves it alone
  t = s;
  CHECK(read(t, "{\"mqtt\":{\"username\":\"other\"}}").empty() && t.mqtt.password == "secret-pass");
  // a uri that is refused (too long) leaves the host, and so the password, alone
  t = s;
  CHECK(has(read(t, "{\"mqtt\":{\"uri\":\"mqtt://" + std::string(200, 'h') + "\"}}"), "mqtt.uri", "too_long") && t.mqtt.uri == s.mqtt.uri && t.mqtt.password == "secret-pass");
  // the backup brokers: an entry of the same user in the same slot keeps its password, and so does one at the same address ...
  s.mqtt.backup = {{"mqtt://b1.local", "u1", "p1"}, {"mqtt://b2.local", "u2", "p2"}};
  t = s;
  CHECK(read(t, "{\"mqtt\":{\"backup\":[{\"uri\":\"mqtt://b1.local:1884\",\"username\":\"u1\"},{\"uri\":\"mqtt://b2.local\",\"username\":\"u2\"}]}}").empty() && t.mqtt.backup[0].password == "p1" && t.mqtt.backup[1].password == "p2");
  // ... but pointing a slot at another host must not carry the password along, as for the main broker
  t = s;
  CHECK(read(t, "{\"mqtt\":{\"backup\":[{\"uri\":\"mqtt://elsewhere.example\",\"username\":\"u1\"}]}}").empty());
  CHECK(t.mqtt.backup.size() == 1 && t.mqtt.backup[0].password.empty());
}

// ---- the limits of the climate and light checks ---------------------------------------------------------------------------------------------------

static void test_climate_limits() {
  CHECK(climate_reading_is_plausible(-50.0f, 0.5f) && climate_reading_is_plausible(85.0f, 100.0f) && climate_reading_is_plausible(20.0f, 50.0f));
  CHECK(!climate_reading_is_plausible(-50.1f, 50.0f) && !climate_reading_is_plausible(85.1f, 50.0f) && !climate_reading_is_plausible(20.0f, 0.0f) && !climate_reading_is_plausible(20.0f, 100.1f) &&
        !climate_reading_is_plausible(20.0f, -1.0f) && !climate_reading_is_plausible(std::nanf(""), 50.0f) && !climate_reading_is_plausible(20.0f, std::nanf("")) &&
        !climate_reading_is_plausible(INFINITY, 50.0f) && !climate_reading_is_plausible(20.0f, INFINITY));
  // the heater stays off for a reading that is not believable, even when the air is about to fog
  HeaterController heater;
  CHECK(heater.update(10.0f, 99.0f) && heater.heating());
  CHECK(!heater.update(10.0f, 0.0f) && !heater.heating());
  CHECK(heater.update(10.0f, 99.0f));
  CHECK(!heater.update(90.0f, 99.0f) && !heater.heating());
  // the thresholds: on strictly under the on margin, off strictly over the off margin
  HeaterController tight(3.0f, 5.0f);
  const float margin_at = 20.0f - dew_point_c(20.0f, 90.0f);   // about 1.4 C: inside the on margin
  CHECK(margin_at < 3.0f && tight.update(20.0f, 90.0f));
  CHECK(tight.update(20.0f, 75.0f));   // margin about 3.9 C: between the margins, stays on
  CHECK(!tight.update(20.0f, 55.0f));  // margin about 9 C: off
  CHECK(!tight.update(20.0f, 75.0f));  // between the margins and off: stays off
  DayNight dn(30.0f, 60.0f);
  CHECK(!dn.update(30.0f) && dn.update(29.99f) && dn.update(60.0f) && !dn.update(60.01f));   // the edges belong to the state already held
  CHECK(!dn.update(std::nanf("")) && !dn.is_night());
}

int main() {
  test_spec();
  test_framer();
  test_slugs_and_reports();
  test_broker_host();
  test_password_follows_its_broker();
  test_climate_limits();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
