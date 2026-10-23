// ARMOR-NODE-RADAR - the settings of a node as a file an administrator can keep and load onto another node, and the checks that make loading one safe. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// The export is {"format":"armor-radar-config","schema":1,"firmware":"x.y.z","config":{...}}: the same document the flash keeps, secrets included (it is for cloning a unit, and the panel offers
// it to administrators only). Loading one is all or nothing: the whole document is read and every setting is checked, against the others as well, before the first thing is stored, and a file that
// belongs to another kind of node, or to a newer layout than this firmware knows, is refused. A bare settings document (no wrapper) is accepted as well, for the panel's own save.
#pragma once
#include <string>
#include <string_view>

#include "node_config.hpp"

namespace armor::config {

constexpr char kExportFormat[] = "armor-radar-config";
constexpr int kExportSchema = 1;
constexpr std::size_t kMaxImportBytes = 24 * 1024;

inline std::string export_document(const Settings& settings, const std::string& firmware) {
  json::Writer w;
  w.begin_object().field("format", kExportFormat).field("schema", kExportSchema).field("firmware", firmware).key("config").raw(to_json(settings, true)).end_object();
  return w.str();
}

// Reads a document to be loaded onto `base`; `out` holds the result only when this returns true (and then every setting has been checked). `problems` says what is wrong otherwise, with the
// path of the setting ("format", "schema", "config" or the path inside the settings).
inline bool import_document(std::string_view text, const Settings& base, Settings& out, Problems& problems) {
  problems.clear();
  if (text.size() > kMaxImportBytes) { problems.push_back({"", "too_large"}); return false; }
  json::Value document;
  if (!json::parse(text, document) || !document.is_object()) { problems.push_back({"", "not_json"}); return false; }
  const json::Value* wrapped = document.get("config");
  if (wrapped == nullptr) {
    if (document.get("format") != nullptr || document.get("schema") != nullptr) { problems.push_back({"config", "required"}); return false; }
    return load_document(document, base, out, problems);   // a bare settings document
  }
  if (const json::Value* format = document.get("format"); format != nullptr && (!format->is_string() || format->text != kExportFormat)) { problems.push_back({"format", "invalid"}); return false; }
  if (const json::Value* schema = document.get("schema"); schema != nullptr) {
    if (!schema->is_number() || schema->number != static_cast<double>(kExportSchema)) { problems.push_back({"schema", "unsupported"}); return false; }
  }
  if (!wrapped->is_object()) { problems.push_back({"config", "invalid"}); return false; }
  return load_document(*wrapped, base, out, problems);
}

}  // namespace armor::config
