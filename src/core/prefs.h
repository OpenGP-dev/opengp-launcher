// SPDX-License-Identifier: GPL-3.0-or-later
//
// The launcher's own settings (user/launcher.toml). The games' settings stay
// in settings.toml and controls.toml, which the games rewrite whole, so
// nothing of the launcher's goes there.
#pragma once

#include <set>
#include <string>

namespace launcher {

// What the launcher does once a game has started.
enum class OnLaunch { Keep, Minimize, Close };

struct Prefs {
  std::string game;                      // the game last chosen (its ID)
  OnLaunch on_launch = OnLaunch::Keep;
  int window_width = 1280, window_height = 760;
  bool news = true;                      // fetch the news feed
  std::string news_url;                  // instead of the built-in one (testing)
  std::set<std::string> news_seen;       // news items the player has seen

  bool operator==(const Prefs&) const = default;
};

// A missing file is not an error (the defaults stay).
bool load_prefs(const std::string& path, Prefs* prefs, std::string* error);
bool save_prefs(const std::string& path, const Prefs& prefs, std::string* error);

const char* on_launch_name(OnLaunch on_launch);  // "keep", "minimize", "close"
bool parse_on_launch(const std::string& name, OnLaunch* on_launch);

}  // namespace launcher
