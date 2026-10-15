// SPDX-License-Identifier: GPL-3.0-or-later
//
// The launcher's core: finding OpenGP, its settings, the news feed, pictures
// from TPL files, the player's files, and following a game that restarts.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <fstream>
#include <sstream>

#include <SDL3/SDL_scancode.h>

#include "core/art.h"
#include "core/game_process.h"
#include "core/home.h"
#include "core/news.h"
#include "core/prefs.h"
#include "core/scancodes.h"
#include "core/user_data.h"

using namespace launcher;

namespace {

// A fresh, empty folder for one test.
struct TempDir {
  fs::path path;
  explicit TempDir(const char* name) : path(fs::temp_directory_path() / name) {
    fs::remove_all(path);
    fs::create_directories(path);
  }
  ~TempDir() {
    std::error_code ec;
    fs::remove_all(path, ec);
  }
};

void write(const fs::path& path, const std::string& text) {
  fs::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << text;
}

std::string read(const fs::path& path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

#ifdef _WIN32
constexpr const char* kExe = ".exe";
#else
constexpr const char* kExe = "";
#endif

const char* kGameToml =
    "id = \"GGPE02\"\ntitle = \"Mario Kart Arcade GP 2\"\ndol_sha1 = \"abc\"\nexecutable = \"opengp-mkgp2\"\n";

}  // namespace

TEST_CASE("keys map to the SDL scancodes the games use") {
#if defined(_WIN32)
  CHECK(sdl_scancode(0x1E, 0) == SDL_SCANCODE_A);
  CHECK(sdl_scancode(0x1C, 0) == SDL_SCANCODE_RETURN);
  CHECK(sdl_scancode(0x11C, 0) == SDL_SCANCODE_KP_ENTER);  // extended
  CHECK(sdl_scancode(0x14B, 0) == SDL_SCANCODE_LEFT);
  CHECK(sdl_scancode(0x45, 0) == SDL_SCANCODE_PAUSE);
#elif defined(__APPLE__)
  CHECK(sdl_scancode(1, 0x00) == SDL_SCANCODE_A);   // kVK_ANSI_A
  CHECK(sdl_scancode(1, 0x7B) == SDL_SCANCODE_LEFT);
#else
  CHECK(sdl_scancode(30 + 8, 0) == SDL_SCANCODE_A);  // KEY_A
  CHECK(sdl_scancode(105 + 8, 0) == SDL_SCANCODE_LEFT);
#endif
  CHECK(sdl_scancode(0, 0xFFFF) == SDL_SCANCODE_UNKNOWN);
}

TEST_CASE("an installed package and a source tree are found") {
  TempDir dir("opengp_launcher_home");
  {
    // A package: sdk/package.toml lists the games.
    const fs::path pkg = dir.path / "pkg";
    write(pkg / "sdk" / "package.toml", "version = \"9.9.9\"\ngames = [\"mkgp2\"]\n[build]\nexecutable_suffix = \"" +
                                            std::string(kExe) + "\"\n");
    write(pkg / "sdk" / "games" / "mkgp2" / "game.toml", kGameToml);
    Home home;
    std::string error;
    REQUIRE_MESSAGE(open_home(pkg, {}, &home, &error), error);
    CHECK(home.package);
    CHECK(home.user_dir == pkg / "user");
    REQUIRE(home.games.size() == 1);
    CHECK(home.games[0].short_title == "GP2");
    CHECK(home.games[0].exe == pkg / (std::string("opengp-mkgp2") + kExe));
    CHECK(default_home(pkg, dir.path / "elsewhere") == pkg);
  }
  {
    // A source tree: games/*/game.toml, the games built somewhere in build/.
    const fs::path src = dir.path / "src";
    write(src / "games" / "mkgp2" / "game.toml", kGameToml);
    write(src / "games" / "mkgp1" / "game.toml",
          "id = \"GGPE01\"\ntitle = \"Mario Kart Arcade GP\"\ndol_sha1 = \"def\"\nexecutable = \"opengp-mkgp1\"\n");
    write(src / "build" / "clang-cl" / "games" / "mkgp2" / (std::string("opengp-mkgp2") + kExe), "");
    Home home;
    std::string error;
    REQUIRE_MESSAGE(open_home(src, dir.path / "u", &home, &error), error);
    CHECK_FALSE(home.package);
    CHECK(home.user_dir == dir.path / "u");
    REQUIRE(home.games.size() == 2);
    CHECK(home.games[0].info.id == "GGPE02");  // the main game first
    CHECK(home.games[0].exe == src / "build" / "clang-cl" / "games" / "mkgp2" / (std::string("opengp-mkgp2") + kExe));
    CHECK(home.games[1].short_title == "GP1");
    CHECK(default_home(src, dir.path / "fallback") == dir.path / "fallback");
  }
  Home home;
  std::string error;
  CHECK_FALSE(open_home(dir.path / "nothing", {}, &home, &error));
}

TEST_CASE("the launcher's settings are kept") {
  TempDir dir("opengp_launcher_prefs");
  const std::string path = utf8(dir.path / "launcher.toml");
  Prefs prefs;
  std::string error;
  CHECK(load_prefs(path, &prefs, &error));  // no file yet: defaults
  CHECK(prefs == Prefs{});
  prefs.game = "GGPE01";
  prefs.on_launch = OnLaunch::Minimize;
  prefs.window_width = 1500;
  prefs.news = false;
  prefs.news_url = "file:///news.toml";
  prefs.news_seen = {"a", "b"};
  REQUIRE(save_prefs(path, prefs, &error));
  Prefs back;
  REQUIRE(load_prefs(path, &back, &error));
  CHECK(back == prefs);
  write(dir.path / "launcher.toml", "this is not toml [");
  CHECK_FALSE(load_prefs(path, &back, &error));
}

TEST_CASE("news items are read, checked and ordered") {
  const std::string feed = R"(
[[news]]
id = "old"
date = 2026-10-01
title = "Old news"
body = "Hello"

[[news]]
id = "new"
date = "2026-10-15"
title = "New news"
tag = "update"
link = "https://example.com/"

[[news]]
id = "pinned"
date = 2026-09-01
title = "Pinned"
pinned = true

[[news]]
id = "bad-link"
date = 2026-10-02
title = "Not https"
link = "http://example.com/"

[[news]]
title = "No id"
date = 2026-10-02

[[news]]
id = "old"
date = 2026-10-03
title = "Same id again"
)";
  std::vector<NewsItem> items;
  int skipped = 0;
  std::string error;
  REQUIRE(parse_news(feed, &items, &skipped, &error));
  CHECK(skipped == 3);
  REQUIRE(items.size() == 3);
  CHECK(items[0].id == "pinned");
  CHECK(items[1].id == "new");
  CHECK(items[1].tag == "update");
  CHECK(items[1].date == "2026-10-15");
  CHECK(items[2].id == "old");
  CHECK(items[2].tag == "announcement");
  CHECK(items[2].body == "Hello");
  CHECK(unread_news(items, {"old"}) == 2);
  CHECK_FALSE(parse_news("not = [toml", &items, &skipped, &error));
  CHECK_FALSE(parse_news("title = \"no news\"", &items, &skipped, &error));
}

TEST_CASE("TPL textures decode, and screens are joined from their tiles") {
  // A TPL with one 4x4 RGB5A3 image, every texel opaque red (0xFC00).
  std::vector<std::uint8_t> tpl = {0x00, 0x20, 0xAF, 0x30, 0, 0, 0, 1, 0, 0, 0, 0x0C,  // magic, 1 image, table at 0x0C
                                   0, 0, 0, 0x14, 0, 0, 0, 0,                          // image header at 0x14, no palette
                                   0, 4, 0, 4, 0, 0, 0, 5, 0, 0, 0, 0x40};             // 4x4, RGB5A3, data at 0x40
  tpl.resize(0x40);
  for (int i = 0; i < 16; ++i) tpl.insert(tpl.end(), {0xFC, 0x00});
  std::vector<Picture> pictures;
  std::string error;
  REQUIRE_MESSAGE(decode_tpl(tpl, &pictures, &error), error);
  REQUIRE(pictures.size() == 1);
  CHECK(pictures[0].width == 4);
  CHECK(pictures[0].height == 4);
  CHECK(pictures[0].at(3, 3) == 0xFF0000FF);  // red, opaque (red in the lowest byte)
  CHECK_FALSE(decode_tpl({1, 2, 3}, &pictures, &error));

  auto solid = [](std::uint32_t w, std::uint32_t h, std::uint32_t colour) {
    Picture p;
    p.width = w, p.height = h;
    p.pixels.assign(std::size_t{w} * h, colour);
    return p;
  };
  // Two rows of 512 + 128 tiles make one 640x4 screen; a lone tile is left out.
  const auto screens = stitch_screens({solid(512, 2, 1), solid(128, 2, 2), solid(512, 2, 3), solid(128, 2, 4),
                                       solid(64, 64, 5)});
  REQUIRE(screens.size() == 1);
  CHECK(screens[0].width == 640);
  CHECK(screens[0].height == 4);
  CHECK(screens[0].at(0, 0) == 1);
  CHECK(screens[0].at(600, 1) == 2);
  CHECK(screens[0].at(600, 3) == 4);

  // A mask's grey becomes the alpha; trimming keeps the visible part.
  Picture logo = solid(4, 4, 0xFF00FF00);
  Picture mask = solid(4, 4, 0xFF000000);
  mask.at(1, 1) = mask.at(2, 2) = 0xFFFFFFFF;
  REQUIRE(apply_mask(&logo, mask));
  CHECK((logo.at(0, 0) >> 24) == 0);
  CHECK((logo.at(1, 1) >> 24) == 0xFF);
  const Picture trimmed = trim(logo);
  CHECK(trimmed.width == 2);
  CHECK(trimmed.height == 2);

  // A pale background around a dark logo goes, and so does a speck of the
  // background's pattern; a pale hole inside the logo stays.
  Picture title = solid(24, 24, 0xFFF0F0F0);
  for (std::uint32_t y = 2; y < 18; ++y)
    for (std::uint32_t x = 2; x < 18; ++x) title.at(x, y) = 0xFF202020;
  title.at(9, 9) = 0xFFF0F0F0;
  title.at(21, 21) = title.at(22, 21) = 0xFF606060;  // a speck
  key_out_background(&title);
  CHECK((title.at(0, 0) >> 24) == 0);
  CHECK((title.at(20, 9) >> 24) == 0);
  CHECK((title.at(2, 2) >> 24) == 0xFF);
  CHECK((title.at(9, 9) >> 24) == 0xFF);
  CHECK((title.at(21, 21) >> 24) == 0);
}

TEST_CASE("cards, backups and a game's own data") {
  TempDir dir("opengp_launcher_user");
  const fs::path user = dir.path;
  std::string error;
  write(user / "cards" / "GGPE02" / "Card 1.bin", "card one");
  write(user / "tribackup_GGPE02.bin", "records");
  write(user / "settings.toml", "[video]\n");
  write(user / "logs" / "GGPE02.log", "log");

  CHECK(list_cards(user, "GGPE02") == std::vector<std::string>{"Card 1"});
  CHECK(rename_card(user, "GGPE02", "Card 1", "Mine", &error));
  CHECK_FALSE(rename_card(user, "GGPE02", "Mine", "bad/name", &error));
  CHECK(duplicate_card(user, "GGPE02", "Mine", &error) == "Mine (copy)");
  CHECK(list_cards(user, "GGPE02") == std::vector<std::string>{"Mine", "Mine (copy)"});
  CHECK(delete_card(user, "GGPE02", "Mine (copy)", &error));
  write(dir.path / "elsewhere" / "Friend.bin", "friend's card");
  CHECK(import_card(user, "GGPE02", dir.path / "elsewhere" / "Friend.bin", &error) == "Friend");
  CHECK(import_card(user, "GGPE02", dir.path / "elsewhere" / "Friend.bin", &error) == "Friend 2");
  CHECK(export_card(user, "GGPE02", "Mine", dir.path / "out.bin", &error));
  CHECK(read(dir.path / "out.bin") == "card one");

  // A backup leaves logs/ out, and restoring puts user/ back as it was.
  const std::string backup = back_up(user, "", &error);
  REQUIRE_MESSAGE(!backup.empty(), error);
  CHECK(fs::exists(user / "backups" / backup / "settings.toml"));
  CHECK_FALSE(fs::exists(user / "backups" / backup / "logs"));
  write(user / "settings.toml", "[video]\nfullscreen = true\n");
  write(user / "cards" / "GGPE02" / "Later.bin", "later");
  REQUIRE_MESSAGE(restore_backup(user, backup, &error), error);
  CHECK(read(user / "settings.toml") == "[video]\n");
  CHECK_FALSE(fs::exists(user / "cards" / "GGPE02" / "Later.bin"));
  CHECK(fs::exists(user / "logs" / "GGPE02.log"));
  CHECK(list_backups(user).size() == 2);  // and the one made before restoring

  CHECK(game_data_files(user, "GGPE02").size() == 1);
  REQUIRE(reset_game_data(user, "GGPE02", &error));
  CHECK(game_data_files(user, "GGPE02").empty());
  CHECK(list_backups(user).size() == 3);
  CHECK(delete_backup(user, backup, &error));
  CHECK(list_backups(user).size() == 2);

  // The next log moves the last one aside.
  const fs::path log = next_log(user, "GGPE02");
  CHECK_FALSE(fs::exists(log));
  CHECK(read(user / "logs" / "GGPE02.1.log") == "log");
}

TEST_CASE("a game that restarts itself is followed until it really ends") {
  TempDir dir("opengp_launcher_process");
  const std::string log = utf8(dir.path / "game.log");
  GameProcess game;
  std::string error;
  REQUIRE_MESSAGE(game.start(FAKE_GAME, {"--exit", "3"}, log, &error), error);
  const GameProcess::Result result = game.wait();
  CHECK(result.exit_code == 3);
  CHECK_FALSE(result.crashed);
  const std::string text = read(log);
  CHECK(text.rfind("fake game", 0) == 0);  // the first process's line, then the restarted one's
  CHECK(text.find("fake game (restarted)") != std::string::npos);

  GameProcess missing;
  CHECK_FALSE(missing.start(utf8(dir.path / "no-such-game"), {}, log, &error));
}
