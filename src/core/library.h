// SPDX-License-Identifier: GPL-3.0-or-later
//
// What the player can do with each game: add its disc image, build it, or
// play it.
#pragma once

#include <string>
#include <vector>

#include "core/home.h"

namespace launcher {

enum class GameState {
  NoImage,    // no disc image of it yet
  NotBuilt,   // an image, but no executable
  OutOfDate,  // built by another version of OpenGP: build it again
  Ready,
};

struct GameStatus {
  GameState state = GameState::NoImage;
  std::string image;  // the disc image the game will use ("" if none)
  std::string note;   // with no image: the one it used to have, if it is gone
};

// Each game's status, in home.games' order. Slow (it reads the disc images
// and runs the games with --version): call it off the UI thread. `skipped`
// gets a line for each file in ISO/ and private/ that is not a usable image,
// and why (a compressed image, another game or region...).
std::vector<GameStatus> check_games(const Home& home, std::vector<std::string>* skipped = nullptr);

// Adds a disc image the player chose: records it in user/images.toml for its
// game (the games and the installer look there first). Fails, saying why, if
// it is not the release of a game OpenGP supports. Sets `game_id`.
bool add_image(const Home& home, const std::string& image, std::string* game_id, std::string* error);

}  // namespace launcher
