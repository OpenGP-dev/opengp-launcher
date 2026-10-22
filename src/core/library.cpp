// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/library.h"

#include "app/paths.h"
#include "disc/disc.h"
#include "os/atomic_file.h"

namespace launcher {

std::vector<GameStatus> check_games(const Home& home) {
  const installer::Reporter quiet{[](const std::string&) {}, [](double) {}};
  const auto images = installer::find_images(home.pkg, {}, quiet);
  std::vector<GameStatus> out;
  for (const Game& g : home.games) {
    GameStatus s;
    // The image recorded for it in this user folder (find_images looks in
    // home/user, which --user-dir may have replaced), else the one found.
    std::error_code ec;
    const std::string remembered = app::remembered_image(home.user_dir, g.info.id);
    if (!remembered.empty() && fs::is_regular_file(os::utf8_path(remembered), ec)) {
      s.image = remembered;
    } else {
      for (const auto& [info, image] : images)
        if (info->id == g.info.id) s.image = image;
    }
    if (s.image.empty()) {
      s.state = GameState::NoImage;
    } else if (!fs::is_regular_file(g.exe, ec)) {
      s.state = GameState::NotBuilt;
    } else if (home.package) {
      s.state = installer::is_installed(home.pkg, g.info) ? GameState::Ready : GameState::OutOfDate;
    } else {
      // Built by CMake from this source tree: as current as the player keeps it.
      s.state = GameState::Ready;
    }
    out.push_back(std::move(s));
  }
  return out;
}

bool add_image(const Home& home, const std::string& image, std::string* game_id, std::string* error) {
  auto fail = [&](const std::string& msg) {
    if (error) *error = msg;
    return false;
  };
  disc::Image img;
  std::string err;
  if (!img.open(image, &err)) return fail(err);
  const std::string sha1 = disc::dol_sha1(img);
  const Game* game = nullptr;
  for (const Game& g : home.games)
    if (g.info.id == img.game_id() || (!sha1.empty() && g.info.dol_sha1 == sha1)) game = &g;
  if (!game) return fail("This is " + img.game_id() + ", not a game OpenGP plays (Mario Kart Arcade GP 2 or GP 1).");
  if (game_id) *game_id = game->info.id;
  if (sha1 != game->info.dol_sha1)
    return fail("This is " + game->info.title + ", but not the release OpenGP supports: its main.dol has the SHA-1 " +
                sha1 + ", and OpenGP needs " + game->info.dol_sha1 + ".");
  if (!app::remember_image(home.user_dir, game->info.id, image, &err)) return fail(err);
  return true;
}

}  // namespace launcher
