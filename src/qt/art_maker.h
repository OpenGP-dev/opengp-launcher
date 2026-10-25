// SPDX-License-Identifier: GPL-3.0-or-later
//
// The launcher's pictures of each game, made from the player's own disc image
// into user/art/<ID>/: logo.png, tile.png (the game's button) and hero_*.png
// (the backgrounds: the courses' preview screens).
#pragma once

#include <QString>
#include <QStringList>

namespace launcher {

struct GameArt {
  bool current = false;  // made from this image by this version
  QString logo, tile;    // file paths ("" if there is none)
  QStringList heroes;
};

// Bump when the pictures change, so that they are made again.
inline constexpr int kArtVersion = 5;

// The pictures made before for this image, if they are current.
GameArt existing_art(const QString& art_dir, const QString& image);

// Makes them (slow: call off the UI thread).
bool make_art(const QString& image, const QString& game_id, const QString& art_dir, QString* error);

}  // namespace launcher
