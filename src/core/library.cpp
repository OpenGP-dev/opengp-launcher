// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/library.h"

#include <cctype>

#include "app/paths.h"
#include "disc/disc.h"
#include "os/atomic_file.h"

namespace launcher {

std::vector<GameStatus> check_games(const Home& home, std::vector<std::string>* skipped) {
  const installer::Reporter report{[skipped](const std::string& line) {
                                     if (skipped) skipped->push_back(line);
                                   },
                                   [](double) {}};
  const auto images = installer::find_images(home.pkg, {}, report);
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
      if (s.image.empty() && !remembered.empty())
        s.note = "Its disc image is no longer at " + remembered + ". Add it again from where it is now.";
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
  // A compressed image, a missing file, or not a disc at all: say which (and
  // how to convert a compressed one).
  disc::Image img;
  std::string err;
  if (!img.open(image, &err)) {
    std::string why = installer::why_not_supported(home.pkg, image);
    const std::string skipped = "; skipped";
    if (why.size() > skipped.size() && why.compare(why.size() - skipped.size(), skipped.size(), skipped) == 0)
      why.resize(why.size() - skipped.size());
    if (!why.empty()) why[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(why[0])));
    return fail(why + ".");
  }
  const std::string sha1 = disc::dol_sha1(img);
  const Game* game = nullptr;
  for (const Game& g : home.games)
    if (g.info.id == img.game_id() || (!sha1.empty() && g.info.dol_sha1 == sha1)) game = &g;
  if (!game) return fail("This is " + img.game_id() + ", not a game OpenGP plays (Mario Kart Arcade GP 2 or GP 1).");
  if (sha1 != game->info.dol_sha1)
    return fail("This is " + game->info.title + ", but not the release OpenGP supports: its main.dol has the SHA-1 " +
                sha1 + ", and OpenGP needs " + game->info.dol_sha1 + ".");
  if (!app::remember_image(home.user_dir, game->info.id, image, &err)) return fail(err);
  if (game_id) *game_id = game->info.id;
  return true;
}

}  // namespace launcher
