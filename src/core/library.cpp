// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/library.h"

#include <algorithm>
#include <cctype>

#include "app/paths.h"
#include "disc/disc.h"
#include "os/atomic_file.h"

namespace launcher {

namespace {

bool compressed(const fs::path& path) {
  std::string ext = utf8(path.extension());
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
  return ext == ".rvz" || ext == ".gcz" || ext == ".wia" || ext == ".ciso" || ext == ".wbfs";
}

// Why a file is not an image of a game OpenGP supports, in the words of
// OpenGP's installer (from 1.0.1; OpenGP 1.0.0 does not say).
std::string why_not_supported(const installer::Package& package, const std::string& image) {
  std::error_code ec;
  const fs::path path = os::utf8_path(image);
  if (!fs::is_regular_file(path, ec)) return "no such file; skipped";
  if (compressed(path)) return "a compressed image; convert it to .iso with Dolphin (see ISO/README.txt)";
  disc::Image img;
  if (!img.open(image, nullptr)) return "not a GameCube or Triforce disc image; skipped";
  std::string supported;
  for (const installer::GameInfo& g : package.games) supported += (supported.empty() ? "" : " or ") + g.id;
  return "its game is " + img.game_id() + ", not a supported release (OpenGP needs " + supported +
         ", the USA releases); skipped";
}

// The images the installer would find (the one each game was installed
// from, then any in ISO/ and private/), with a line in `skipped` for each
// file there that is not one. Not OpenGP 1.0.0's find_images, which throws
// at a folder it cannot read.
std::vector<std::pair<const installer::GameInfo*, std::string>> find_images(const installer::Package& package,
                                                                            std::vector<std::string>* skipped) {
  std::vector<std::pair<const installer::GameInfo*, std::string>> found;
  auto have = [&](const installer::GameInfo* g) {
    return std::any_of(found.begin(), found.end(), [&](const auto& f) { return f.first == g; });
  };
  const fs::path user_dir = package.home / "user";
  for (const installer::GameInfo& g : package.games) {
    const std::string image = app::remembered_image(user_dir, g.id);
    std::error_code ec;
    if (!image.empty() && fs::is_regular_file(os::utf8_path(image), ec) && installer::identify_image(package, image) == &g)
      found.emplace_back(&g, image);
  }
  for (const char* folder : {"ISO", "private"}) {
    std::error_code ec;
    const fs::path dir = package.home / folder;
    if (!fs::is_directory(dir, ec)) continue;
    for (fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
      std::error_code e;
      if (!it->is_regular_file(e)) continue;
      std::string ext = utf8(it->path().extension());
      std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
      if (ext != ".iso" && ext != ".gcm" && !compressed(it->path())) continue;
      const std::string image = utf8(it->path());
      if (const installer::GameInfo* g = installer::identify_image(package, image)) {
        if (!have(g)) found.emplace_back(g, image);
      } else if (skipped) {
        skipped->push_back(utf8(it->path().filename()) + ": " + why_not_supported(package, image));
      }
    }
  }
  return found;
}

}  // namespace

std::vector<GameStatus> check_games(const Home& home, std::vector<std::string>* skipped) {
  const auto images = find_images(home.pkg, skipped);
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
    std::string why = why_not_supported(home.pkg, image);
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
  if (game_id) *game_id = game->info.id;
  if (sha1 != game->info.dol_sha1)
    return fail("This is " + game->info.title + ", but not the release OpenGP supports: its main.dol has the SHA-1 " +
                sha1 + ", and OpenGP needs " + game->info.dol_sha1 + ".");
  if (!app::remember_image(home.user_dir, game->info.id, image, &err)) return fail(err);
  return true;
}

}  // namespace launcher
