// ARMOR-NODE-RADAR - which file of a GitHub release is the firmware of THIS board, and which is its checksum. Pure: tested on a computer.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// A release attaches one standalone application image per board, `armor_radar-<board>.bin` (for example `armor_radar-s3-wifi.bin`), and next to each its SHA-256, `<image>.sha256`, and the
// signature of its manifest, `<image>.sig2` (see fw_trust.hpp; the older `<image>.sig`, of the hash alone, is for the nodes before that check and is not used here).
// A release that predates that naming has only `armor_radar.bin`: the image of the project's DEFAULT board (s3-eth, or lcd7box for the touch panel). A node finds the image built for
// its own board; the plain name is accepted only on the default board, and a node of any other board never installs an image that is not its own.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "json.hpp"

namespace armor::release {

constexpr const char* kAssetPrefix = "armor_radar";

// Only what GitHub itself publishes for this repository is downloaded and installed: https://github.com/<owner>/<repo>/releases/download/<tag>/<file>, with nothing but one tag and one file
// name after it. (An address that comes out of the release's JSON is still checked: a node that asks for any address it is handed can be made to install from anywhere.)
inline bool is_official_url(const std::string& url, const std::string& repo) {
  const std::string prefix = "https://github.com/" + repo + "/releases/download/";
  if (repo.empty() || url.compare(0, prefix.size(), prefix) != 0) return false;
  std::size_t slashes = 0;
  for (std::size_t i = prefix.size(); i < url.size(); ++i) {
    const char c = url[i];
    if (c == '/') ++slashes;
    else if (c == '?' || c == '#' || c == '\\' || c == '@' || c <= ' ' || c == '%') return false;
  }
  return slashes == 1 && url.size() > prefix.size() + 3 && url.find("..") == std::string::npos && url.back() != '/';
}

// The buffer the HTTP client keeps for the headers of an answer. GitHub redirects a download to a storage address whose headers add up to more than 5 KB (a signed Location of
// about a thousand characters, a security policy, cookies): a buffer of 4 KB made the client give up with "out of buffer". The sdkconfig sends an allocation of 4096 bytes or less to
// INTERNAL memory (there is only some 65 to 80 KB of it free) and a bigger one to the external RAM first, so 20480 bytes is both roomy and free of cost for the internal heap. A board with no external RAM (or none free) takes the smallest buffer that holds those headers.
constexpr int kHeaderBufferExternal = 20480;
constexpr int kHeaderBufferInternal = 6144;
// The REQUEST buffer of the HTTP client (its default is 512 bytes): after GitHub's redirect the request line alone carries an address of about a thousand characters, and the client
// refuses it with "Out of buffer" when it does not fit. 4096 holds that address, the host and the other request headers with room for a longer one.
constexpr int kRequestBuffer = 4096;
static_assert(kRequestBuffer >= 2048, "the request buffer must hold GitHub's long redirect address");
inline int header_buffer_size(std::size_t free_external_bytes) { return free_external_bytes >= 2u * static_cast<std::size_t>(kHeaderBufferExternal) ? kHeaderBufferExternal : kHeaderBufferInternal; }

// What to tell when a file of a release (`what` is "checksum" or "signature") could not be read: the real reason, never "the release has none" (that is only for a release that does
// not carry the file). `problem` is what the download said: connect_failed, read_error, client_init, http_<status> or too_large.
inline std::string fetch_error(const std::string& problem, const std::string& what) {
  if (problem.compare(0, 5, "http_") == 0) return problem;
  if (problem == "too_large") return what + "_too_large";
  return what + "_unreachable";
}

inline std::string asset_name(const char* board_id) { return std::string(kAssetPrefix) + "-" + (board_id != nullptr ? board_id : "") + ".bin"; }

// The plain name, for the board whose image a release without the board's name carries; "" for every other board.
inline std::string legacy_asset_name(const char* board_id) {
  const std::string board = board_id != nullptr ? board_id : "";
  return board == "s3-eth" || board == "lcd7box" ? std::string(kAssetPrefix) + ".bin" : "";
}

struct Picked {
  std::string image_url;      // empty: the release has no image for this board
  std::string checksum_url;   // empty: it has the image but not the checksum file next to it
  std::string signature_url;  // empty: it has the image but not the signature of its manifest (`<image>.sig2`)
  bool legacy = false;        // the image is the one with the plain name
};

// `assets` is the "assets" array of the release's JSON (each with "name" and "browser_download_url"). The image built for the board wins; the plain name is the fallback.
inline Picked pick(const json::Value* assets, const char* board_id) {
  Picked picked;
  if (assets == nullptr || !assets->is_array()) return picked;
  const std::string wanted = asset_name(board_id), legacy = legacy_asset_name(board_id);
  std::string wanted_url, wanted_sum, wanted_sig, legacy_url, legacy_sum, legacy_sig;
  for (const json::Value& asset : assets->items) {
    const std::string name = asset.string_or("name", ""), url = asset.string_or("browser_download_url", "");
    if (name == wanted) wanted_url = url;
    else if (name == wanted + ".sha256") wanted_sum = url;
    else if (name == wanted + ".sig2") wanted_sig = url;
    else if (!legacy.empty() && name == legacy) legacy_url = url;
    else if (!legacy.empty() && name == legacy + ".sha256") legacy_sum = url;
    else if (!legacy.empty() && name == legacy + ".sig2") legacy_sig = url;
  }
  if (!wanted_url.empty()) { picked.image_url = wanted_url; picked.checksum_url = wanted_sum; picked.signature_url = wanted_sig; }
  else if (!legacy_url.empty()) { picked.image_url = legacy_url; picked.checksum_url = legacy_sum; picked.signature_url = legacy_sig; picked.legacy = true; }
  return picked;
}

// GitHub's answer about a release is long and grows with it (every file carries its uploader, and the notes are in it): a release with a dozen files and long notes passed the 24 kB
// the node used to keep, and the look for a new version answered "too large". This reads the answer piece by piece as it arrives and keeps only what a node needs - the tag and, for
// every file, its name and address - so the memory it takes does not grow with the notes. `pruned()` is that, as a small JSON document for json::parse and pick(). It follows the
// strings and the brackets; what it hands over is then parsed strictly.
class ReleaseFilter {
 public:
  static constexpr std::size_t kMaxAnswer = 512 * 1024;   // anything bigger is not trusted
  static constexpr std::size_t kMaxAssets = 64;
  static constexpr std::size_t kMaxKept = 512;            // one tag, name or address
  static constexpr std::size_t kMaxDepth = 16;

  // False when the answer cannot be a release's (not an object, brackets that do not match, too long, too many files, a value that is wanted and too long): it is then given up.
  bool feed(const char* data, std::size_t length) {
    if (failed_) return false;
    if (length > kMaxAnswer - total_) { too_large_ = true; return fail(); }
    total_ += length;
    for (std::size_t i = 0; i < length; ++i) if (!take(data[i])) return fail();
    return true;
  }
  bool complete() const { return !failed_ && closed_; }
  bool too_large() const { return too_large_; }
  std::string pruned() const {
    std::string out = "{";
    if (has_tag_) out += "\"tag_name\":\"" + tag_ + "\",";
    out += "\"assets\":[";
    for (std::size_t i = 0; i < assets_.size(); ++i) {
      out += std::string(i != 0 ? "," : "") + "{\"name\":\"" + assets_[i].name + "\",\"browser_download_url\":\"" + assets_[i].url + "\"}";
    }
    return out + "]}";
  }

 private:
  struct Asset { std::string name, url; };

  bool fail() { failed_ = true; return false; }

  bool take(char c) {
    if (in_string_) {
      if (!escaped_ && c == '"') { in_string_ = false; return string_ended(); }
      escaped_ = !escaped_ && c == '\\';
      if (text_.size() < kMaxKept) text_ += c; else cut_ = true;   // the escapes stay as they came: json::parse reads them later
      return true;
    }
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return true;
    if (closed_) return false;
    if (depth_ == 0) return c == '{' && open('{');
    switch (c) {
      case '"': in_string_ = true; escaped_ = false; cut_ = false; text_.clear(); return true;
      case '{': case '[': return open(c);
      case '}': case ']':
        if (stack_[depth_ - 1] != (c == '}' ? '{' : '[')) return false;
        if (--depth_ == 0) closed_ = true;
        key_next_ = false;
        return true;
      case ',': key_next_ = stack_[depth_ - 1] == '{'; return true;
      case ':': key_next_ = false; return true;
      default: return true;   // numbers and true, false, null: nothing the node needs
    }
  }

  bool open(char bracket) {
    if (depth_ >= kMaxDepth) return false;
    if (bracket == '{' && depth_ == 2 && in_assets()) {
      if (assets_.size() >= kMaxAssets) return false;
      assets_.emplace_back();
      asset_key_.clear();
    }
    stack_[depth_++] = bracket;
    key_next_ = bracket == '{';
    return true;
  }

  bool in_assets() const { return depth_ >= 2 && stack_[1] == '[' && root_key_ == "assets"; }

  bool string_ended() {
    if (key_next_) {   // the name of a member
      if (depth_ == 1) root_key_ = cut_ ? std::string() : text_;
      else if (depth_ == 3) asset_key_ = cut_ ? std::string() : text_;
      return true;
    }
    if (depth_ == 1 && root_key_ == "tag_name") {
      if (cut_ || has_tag_) return false;
      tag_ = text_;
      has_tag_ = true;
    } else if (depth_ == 3 && stack_[2] == '{' && in_assets() && !assets_.empty()) {
      std::string* wanted = asset_key_ == "name" ? &assets_.back().name : asset_key_ == "browser_download_url" ? &assets_.back().url : nullptr;
      if (wanted != nullptr) {
        if (cut_ || !wanted->empty()) return false;
        *wanted = text_;
      }
    }
    return true;
  }

  std::size_t total_ = 0;
  char stack_[kMaxDepth] = {};
  std::size_t depth_ = 0;
  bool in_string_ = false, escaped_ = false, cut_ = false, key_next_ = false, closed_ = false, failed_ = false, too_large_ = false, has_tag_ = false;
  std::string text_, root_key_, asset_key_, tag_;
  std::vector<Asset> assets_;
};

}  // namespace armor::release
