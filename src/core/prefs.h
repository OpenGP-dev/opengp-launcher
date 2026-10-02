// SPDX-License-Identifier: GPL-3.0-or-later
//
// The launcher's own settings (user/launcher.toml). The games' settings stay
// in settings.toml and controls.toml, which the games rewrite whole, so
// nothing of the launcher's goes there.
#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>

namespace launcher {

// What the launcher does once a game has started.
enum class OnLaunch { Keep, Minimize, Close };

// The last check of a game's disc image against the known good dumps
// (dump_check.h). It holds while the image is the same file: the same path,
// size and modification time.
struct ImageCheck {
  std::string image;
  std::int64_t size = 0, modified = 0;
  std::string verdict;  // "good", "bad" (a known bad dump) or "unknown"
  std::string dump, note;

  bool operator==(const ImageCheck&) const = default;
};

struct Prefs {
  std::string game;                      // the game last chosen (its ID)
  OnLaunch on_launch = OnLaunch::Keep;
  int window_width = 1280, window_height = 760;
  bool news = true;                      // fetch the news feed
  std::string news_url;                  // instead of the built-in one (testing)
  std::set<std::string> news_seen;       // news items the player has seen
  std::map<std::string, ImageCheck> image_checks;  // by game ID

  bool operator==(const Prefs&) const = default;
};

// A missing file is not an error (the defaults stay).
bool load_prefs(const std::string& path, Prefs* prefs, std::string* error);
bool save_prefs(const std::string& path, const Prefs& prefs, std::string* error);

const char* on_launch_name(OnLaunch on_launch);  // "keep", "minimize", "close"
bool parse_on_launch(const std::string& name, OnLaunch* on_launch);

}  // namespace launcher
