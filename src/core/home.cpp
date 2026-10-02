// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/home.h"

#include <algorithm>
#include <fstream>

#define TOML_EXCEPTIONS 0
#include <toml.hpp>

#include "core/source_build.h"

namespace launcher {

namespace {

#ifdef _WIN32
constexpr const char* kExeSuffix = ".exe";
#else
constexpr const char* kExeSuffix = "";
#endif

std::string short_title(const std::string& id, const std::string& title) {
  if (id == "GGPE02") return "GP2";
  if (id == "GGPE01") return "GP1";
  return title;
}

// A source tree's games: games/<game>/game.toml, as the package has them.
bool read_source_games(const fs::path& dir, installer::Package* pkg, std::string* error) {
  std::error_code ec;
  for (const auto& entry : fs::directory_iterator(dir / "games", ec)) {
    const fs::path file = entry.path() / "game.toml";
    if (!fs::is_regular_file(file, ec)) continue;
    std::ifstream f(file, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    toml::parse_result parsed = toml::parse(text);
    if (!parsed) {
      if (error) *error = utf8(file) + ": " + std::string(parsed.error().description());
      return false;
    }
    const toml::table& t = parsed.table();
    installer::GameInfo g;
    g.dir = entry.path();
    g.id = t["id"].value_or(std::string());
    g.title = t["title"].value_or(std::string());
    g.dol_sha1 = t["dol_sha1"].value_or(std::string());
    g.executable = t["executable"].value_or(std::string());
    if (!g.id.empty() && !g.executable.empty()) pkg->games.push_back(std::move(g));
  }
  if (pkg->games.empty()) {
    if (error) *error = utf8(dir) + " has no games (games/*/game.toml)";
    return false;
  }
  // The main game first: GP 2 (GGPE02) before GP 1 (GGPE01).
  std::sort(pkg->games.begin(), pkg->games.end(), [](const auto& a, const auto& b) { return a.id > b.id; });
  pkg->executable_suffix = kExeSuffix;
  return true;
}

// The newest build of a game in a source tree: build/<preset>/games/<game>/.
fs::path find_source_build(const fs::path& dir, const installer::GameInfo& game, const std::string& suffix) {
  fs::path best;
  fs::file_time_type best_time{};
  std::error_code ec;
  for (const auto& build : fs::directory_iterator(dir / "build", ec)) {
    const fs::path exe = build.path() / "games" / game.dir.filename() / (game.executable + suffix);
    std::error_code e;
    if (!fs::is_regular_file(exe, e)) continue;
    const auto time = fs::last_write_time(exe, e);
    if (best.empty() || time > best_time) best = exe, best_time = time;
  }
  // None yet: where the default preset puts it.
  return best.empty() ? dir / "build" / default_preset() / "games" / game.dir.filename() / (game.executable + suffix) : best;
}

}  // namespace

std::string utf8(const fs::path& path) {
  const std::u8string s = path.u8string();
  return std::string(s.begin(), s.end());
}

const Game* Home::game(const std::string& id) const {
  for (const Game& g : games)
    if (g.info.id == id) return &g;
  return nullptr;
}

bool open_home(const fs::path& dir, const fs::path& user_dir, Home* home, std::string* error) {
  Home h;
  h.dir = dir;
  h.user_dir = user_dir.empty() ? dir / "user" : user_dir;
  std::error_code ec;
  h.package = fs::is_regular_file(dir / "sdk" / "package.toml", ec);
  if (h.package) {
    if (!installer::load_package(dir, &h.pkg, error)) return false;
  } else {
    h.pkg.home = dir;
    if (!read_source_games(dir, &h.pkg, error)) {
      if (error) *error = utf8(dir) + " is neither an OpenGP package nor an OpenGP source tree (" + *error + ")";
      return false;
    }
  }
  for (const installer::GameInfo& info : h.pkg.games) {
    Game g;
    g.info = info;
    g.short_title = short_title(info.id, info.title);
    g.exe = h.package ? dir / (info.executable + h.pkg.executable_suffix)
                      : find_source_build(dir, info, h.pkg.executable_suffix);
    h.games.push_back(std::move(g));
  }
  *home = std::move(h);
  return true;
}

fs::path default_home(const fs::path& exe_dir, const fs::path& fallback) {
  std::error_code ec;
  if (fs::is_regular_file(exe_dir / "sdk" / "package.toml", ec)) return exe_dir;
  // In a macOS app bundle: the folder OpenGP.app is in.
  if (exe_dir.filename() == "MacOS" && exe_dir.parent_path().filename() == "Contents") {
    const fs::path outside = exe_dir.parent_path().parent_path().parent_path();
    if (fs::is_regular_file(outside / "sdk" / "package.toml", ec)) return outside;
  }
  return fallback;
}

}  // namespace launcher
