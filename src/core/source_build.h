// SPDX-License-Identifier: GPL-3.0-or-later
//
// Building a game in a source tree (development) with CMake, as a package
// builds it with the installer: the game's main.dol goes from the player's
// disc image into orig/<id>/sys/main.dol (where tools/setup.py puts it), then
// CMake configures the tree (it builds the games whose main.dol is there) and
// builds the game's executable where the launcher looks for it (Game::exe).
#pragma once

#include <string>
#include <vector>

#include "core/home.h"

namespace launcher {

// The preset a source tree is configured with when it has no build yet.
const char* default_preset();

// Writes the game's main.dol from `image` into the source tree, checked
// against the release the game needs.
bool extract_dol(const Home& home, const Game& game, const std::string& image, std::string* error);

struct SourceBuild {
  fs::path build_dir;                              // build/<preset>/
  std::vector<std::vector<std::string>> commands;  // configure, then build (run in home.dir)
};

// The commands that build `game`. `cmake` is the CMake to run, `ninja` the
// Ninja a new build directory is configured with ("" for CMake's own choice).
SourceBuild source_build(const Home& home, const Game& game, const std::string& cmake, const std::string& ninja);

// What Ninja's progress line ("[12/345] Building CXX object ...") says is
// done, from 0 to 1, or -1 if `line` is not one.
double ninja_progress(const std::string& line);

}  // namespace launcher
