// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/prefs.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "os/atomic_file.h"

#define TOML_EXCEPTIONS 0
#include <toml.hpp>

namespace launcher {

const char* on_launch_name(OnLaunch on_launch) {
  switch (on_launch) {
    case OnLaunch::Minimize: return "minimize";
    case OnLaunch::Close: return "close";
    default: return "keep";
  }
}

bool parse_on_launch(const std::string& name, OnLaunch* on_launch) {
  for (OnLaunch o : {OnLaunch::Keep, OnLaunch::Minimize, OnLaunch::Close})
    if (name == on_launch_name(o)) {
      *on_launch = o;
      return true;
    }
  return false;
}

bool load_prefs(const std::string& path, Prefs* prefs, std::string* error) {
  std::ifstream f(os::utf8_path(path), std::ios::binary);
  if (!f) return true;
  const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  toml::parse_result parsed = toml::parse(text);
  if (!parsed) {
    if (error) *error = path + ": " + std::string(parsed.error().description());
    return false;
  }
  const toml::table& t = parsed.table();
  Prefs p = *prefs;
  p.game = t["launcher"]["game"].value_or(p.game);
  parse_on_launch(t["launcher"]["on_launch"].value_or(std::string()), &p.on_launch);
  p.window_width = std::clamp(t["launcher"]["window_width"].value_or(p.window_width), 640, 16384);
  p.window_height = std::clamp(t["launcher"]["window_height"].value_or(p.window_height), 400, 16384);
  p.news = t["news"]["enabled"].value_or(p.news);
  p.news_url = t["news"]["url"].value_or(p.news_url);
  if (const toml::array* seen = t["news"]["seen"].as_array()) {
    p.news_seen.clear();
    for (const toml::node& n : *seen)
      if (auto s = n.value<std::string>()) p.news_seen.insert(*s);
  }
  if (const toml::table* checks = t["image_checks"].as_table()) {
    p.image_checks.clear();
    for (const auto& [id, node] : *checks) {
      const toml::table* c = node.as_table();
      if (!c) continue;
      ImageCheck ic;
      ic.image = (*c)["image"].value_or(std::string());
      ic.size = (*c)["size"].value_or(std::int64_t{0});
      ic.modified = (*c)["modified"].value_or(std::int64_t{0});
      ic.verdict = (*c)["verdict"].value_or(std::string());
      ic.dump = (*c)["dump"].value_or(std::string());
      ic.note = (*c)["note"].value_or(std::string());
      p.image_checks[std::string(id.str())] = std::move(ic);
    }
  }
  *prefs = std::move(p);
  return true;
}

bool save_prefs(const std::string& path, const Prefs& prefs, std::string* error) {
  toml::array seen;
  for (const std::string& id : prefs.news_seen) seen.push_back(id);
  toml::table t{
      {"launcher", toml::table{{"game", prefs.game},
                               {"on_launch", on_launch_name(prefs.on_launch)},
                               {"window_width", prefs.window_width},
                               {"window_height", prefs.window_height}}},
      {"news", toml::table{{"enabled", prefs.news}, {"url", prefs.news_url}, {"seen", std::move(seen)}}},
  };
  toml::table checks;
  for (const auto& [id, c] : prefs.image_checks)
    checks.insert(id, toml::table{{"image", c.image},
                                  {"size", c.size},
                                  {"modified", c.modified},
                                  {"verdict", c.verdict},
                                  {"dump", c.dump},
                                  {"note", c.note}});
  if (!checks.empty()) t.insert("image_checks", std::move(checks));
  std::ostringstream out;
  out << "# The OpenGP launcher's settings (the games' are in settings.toml).\n\n" << t << "\n";
  const std::string text = out.str();
  if (!os::write_file_atomically(path, text.data(), text.size())) {
    if (error) *error = "cannot write " + path;
    return false;
  }
  return true;
}

}  // namespace launcher
