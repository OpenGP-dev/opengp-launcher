// SPDX-License-Identifier: GPL-3.0-or-later
//
// The OpenGP the launcher looks after: an installed package (the launcher is
// in its folder, and builds the games there from the player's disc images),
// or a source tree (development: CMake builds the games).
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "installer/installer.h"

namespace launcher {

namespace fs = std::filesystem;

struct Game {
  installer::GameInfo info;  // id, title, dol_sha1, executable name
  std::string short_title;   // "GP2"
  fs::path exe;              // the game's executable (it need not be built yet)
};

struct Home {
  fs::path dir;
  fs::path user_dir;
  bool package = false;      // an installed package, which can build its games
  installer::Package pkg;    // a source tree's has its games only (from games/*/game.toml)
  std::vector<Game> games;   // the main game (GP 2) first

  const Game* game(const std::string& id) const;
};

// Opens the OpenGP in `dir`; `user_dir` empty for dir/user.
bool open_home(const fs::path& dir, const fs::path& user_dir, Home* home, std::string* error);

// The OpenGP to look after when none is given: the launcher's own folder if
// that is a package, else `fallback` if it exists (in development, the source
// tree the launcher was built with), else the launcher's own folder.
fs::path default_home(const fs::path& exe_dir, const fs::path& fallback);

// Whether a package folder can be written to (the games, their builds and
// the saves all go there): if not, `error` says where to move it, in the
// words of OpenGP's installer::check_writable and with the same reason (in a
// new package, with no build/ yet, the permission, not the missing folder).
bool check_writable(const fs::path& dir, std::string* error);

// A path as UTF-8 text (std::filesystem would use the system's code page on
// Windows).
std::string utf8(const fs::path& path);

}  // namespace launcher
