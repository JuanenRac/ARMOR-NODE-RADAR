// ARMOR-RADAR - the node's numbers in the text format that Prometheus reads (GET /metrics, administrators only). Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// Format: "# HELP name text", "# TYPE name gauge|counter", then "name{label=\"value\"} number". Names and labels are checked here (letters, digits and underscore), label values are escaped, and a number
// that is not finite is left out rather than written as something the reader would reject.
#pragma once
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace armor::metrics {

inline bool name_is_valid(const std::string& name) {
  if (name.empty()) return false;
  for (std::size_t i = 0; i < name.size(); ++i) {
    const char c = name[i];
    const bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == ':';
    if (!(letter || (i > 0 && c >= '0' && c <= '9'))) return false;
  }
  return true;
}

inline std::string escape_label(const std::string& value) {
  std::string out;
  for (char c : value) {
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (static_cast<unsigned char>(c) >= 0x20) out += c;
  }
  return out;
}

using Labels = std::vector<std::pair<std::string, std::string>>;

class Page {
 public:
  // Declares a metric (once); later add() calls with the same name write its samples under it.
  void family(const std::string& name, const std::string& help, const char* type) {
    if (!name_is_valid(name)) { current_.clear(); return; }
    current_ = name;
    text_ += "# HELP " + name + " " + clean_help(help) + "\n# TYPE " + name + " " + type + "\n";
  }
  void add(double value, const Labels& labels = Labels{}) {
    if (current_.empty() || !std::isfinite(value)) return;
    text_ += current_;
    if (!labels.empty()) {
      text_ += '{';
      bool first = true;
      for (const auto& label : labels) {
        if (!name_is_valid(label.first)) continue;
        if (!first) text_ += ',';
        first = false;
        text_ += label.first + "=\"" + escape_label(label.second) + "\"";
      }
      text_ += '}';
    }
    char number[40];
    if (value == std::floor(value) && std::fabs(value) < 9.0e15) std::snprintf(number, sizeof number, " %.0f\n", value);
    else std::snprintf(number, sizeof number, " %.6g\n", value);
    text_ += number;
  }
  void gauge(const std::string& name, const std::string& help, double value, const Labels& labels = Labels{}) { family(name, help, "gauge"); add(value, labels); }
  void counter(const std::string& name, const std::string& help, double value, const Labels& labels = Labels{}) { family(name, help, "counter"); add(value, labels); }
  const std::string& str() const { return text_; }

 private:
  static std::string clean_help(const std::string& help) {
    std::string out;
    for (char c : help) out += (c == '\n' || c == '\\') ? ' ' : c;
    return out;
  }
  std::string text_;
  std::string current_;
};

}  // namespace armor::metrics
