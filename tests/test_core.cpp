// SPDX-License-Identifier: GPL-3.0-or-later
//
// The launcher's core: finding OpenGP, its settings, the news feed, pictures
// from TPL files, the player's files, and following a game that restarts.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <chrono>
#include <fstream>
#include <sstream>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <SDL3/SDL_scancode.h>

#include "core/art.h"
#include "core/dump_check.h"
#include "core/game_end.h"
#include "core/game_process.h"
#include "core/home.h"
#include "core/library.h"
#include "core/news.h"
#include "core/prefs.h"
#include "core/scancodes.h"
#include "core/source_build.h"
#include "core/user_data.h"
#include "app/paths.h"
#include "disc/sha1.h"

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
                                            std::string(kExe) +
                                            "\"\ncommon_sources = [\"main.cpp\"]\ngame_sources = [\"patches.cpp\"]\n");
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
    CHECK(home.games[1].exe ==  // not built yet: where the default preset puts it
          src / "build" / default_preset() / "games" / "mkgp1" / (std::string("opengp-mkgp1") + kExe));
    fs::create_directories(dir.path / "fallback");
    CHECK(default_home(src, dir.path / "fallback") == dir.path / "fallback");
    CHECK(default_home(src, dir.path / "not there") == src);  // a package's launcher, moved out of it
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
  prefs.image_checks["GGPE02"] = {"/games/gp2.iso", 503316480, 1234567890123, "good", "GP 2 [Feb 7 2007]", ""};
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

TEST_CASE("a game started elsewhere is running while it holds its lock") {
  TempDir dir("opengp_launcher_running");
  const fs::path user = dir.path / "user";
  // No lock file: no game, and no file made.
  CHECK_FALSE(game_running(user, "GGPE02"));
  CHECK_FALSE(fs::exists(user / ".lock-GGPE02"));
  // One left by a game that has ended.
  write(user / ".lock-GGPE01", "");
  CHECK_FALSE(game_running(user, "GGPE01"));
  // Held as a running game holds it (until this test ends).
  REQUIRE(app::lock_user_dir(user, "GGPE02"));
  CHECK(game_running(user, "GGPE02"));
  CHECK_FALSE(game_running(user, "GGPE01"));
}

TEST_CASE("a source tree's game is built with CMake from the player's image") {
  TempDir dir("opengp_launcher_source_build");
  const fs::path src = dir.path / "src";
  write(src / "games" / "mkgp2" / "game.toml", kGameToml);

  // A disc image: the header (ID, GameCube magic word, where main.dol is),
  // then a main.dol of just its 0x100-byte header (no sections).
  std::string image(0x440 + 0x100, '\0');
  image.replace(0, 6, "GGPE02");
  image.replace(0x1C, 4, "\xC2\x33\x9F\x3D");
  image.replace(0x420, 4, std::string("\0\0\x04\x40", 4));
  image[0x440 + 0xE0] = 0x12;  // the entry point: something to tell it apart
  const std::string dol = image.substr(0x440);
  write(dir.path / "game.iso", image);

  Home home;
  std::string error;
  REQUIRE_MESSAGE(open_home(src, {}, &home, &error), error);
  Game game = home.games[0];

  // Only the supported release.
  CHECK_FALSE(extract_dol(home, game, (dir.path / "game.iso").string(), &error));
  CHECK(error.find("not the supported release") != std::string::npos);
  game.info.dol_sha1 = disc::to_hex(disc::sha1(dol.data(), dol.size()));
  REQUIRE_MESSAGE(extract_dol(home, game, (dir.path / "game.iso").string(), &error), error);
  CHECK(read(src / "orig" / "GGPE02" / "sys" / "main.dol") == dol);

  // Not configured yet: by the preset, then the game's target.
  SourceBuild b = source_build(home, game, "cmake", "/bin/ninja");
  CHECK(b.build_dir == src / "build" / default_preset());
  REQUIRE(b.commands.size() == 2);
  CHECK(b.commands[0] == std::vector<std::string>{"cmake", "--preset", default_preset(), "-DOPENGP_BUILD_GAMES=ON",
                                                  "-DCMAKE_MAKE_PROGRAM=/bin/ninja"});
  CHECK(b.commands[1] == std::vector<std::string>{"cmake", "--build", utf8(b.build_dir), "--target", "opengp-mkgp2"});

  // Configured before (here by hand, not by a preset): that build directory again.
  game.exe = src / "build" / "mine" / "games" / "mkgp2" / "opengp-mkgp2";
  write(src / "build" / "mine" / "CMakeCache.txt", "");
  b = source_build(home, game, "cmake", "");
  CHECK(b.build_dir == src / "build" / "mine");
  REQUIRE(b.commands.size() == 2);
  CHECK(b.commands[0] ==
        std::vector<std::string>{"cmake", "-S", utf8(src), "-B", utf8(b.build_dir), "-DOPENGP_BUILD_GAMES=ON"});

  CHECK(ninja_progress("[25/100] Building CXX object games/mkgp2/generated/recomp_001.cpp.o") == doctest::Approx(0.25));
  CHECK(ninja_progress("[100/100] Linking CXX executable games/mkgp2/opengp-mkgp2") == doctest::Approx(1.0));
  CHECK(ninja_progress("-- Configuring done") < 0);
  CHECK(ninja_progress("[0/0]") < 0);
}

TEST_CASE("disc images are checked against the known good dumps") {
  // The streaming SHA-1 agrees with OpenGP's, across block boundaries.
  std::string data;
  for (int i = 0; i < 1000; ++i) data += static_cast<char>(i * 7);
  for (std::size_t n : {0u, 1u, 55u, 56u, 63u, 64u, 65u, 119u, 1000u}) {
    Sha1 sha;
    sha.update(data.data(), n / 3);
    sha.update(data.data() + n / 3, n - n / 3);
    CHECK(sha.hex() == disc::to_hex(disc::sha1(data.data(), n)));
  }
  Sha1 abc;
  abc.update("abc", 3);
  CHECK(abc.hex() == "a9993e364706816aba3e25717850c26c9cd0d89d");  // FIPS 180 example

  TempDir dir("opengp_launcher_dump_check");
  const std::string image = utf8(dir.path / "gp.iso");
  DumpCheck check;
  std::string error;
  write(dir.path / "gp.iso", std::string(0x1000, '\0'));
  CHECK_FALSE(check_dump(image, "GXXX01", {}, nullptr, &check, &error));  // no known dump
  REQUIRE_MESSAGE(check_dump(image, "GGPE01", {}, nullptr, &check, &error), error);
  CHECK(check.verdict == DumpVerdict::Unknown);
  CHECK(check.note.find("cut short") != std::string::npos);
  CHECK(check.body_sha1.empty());

  // As big as GP's dump, plus 16 bytes: sparse, so cheap to make.
  constexpr std::uint64_t kGpSize = 0x15000000;
  auto make = [&](const std::string& tail) {
    fs::remove(dir.path / "gp.iso");
    std::ofstream f(dir.path / "gp.iso", std::ios::binary);
    f.seekp(static_cast<std::streamoff>(kGpSize));
    f.write(tail.data(), static_cast<std::streamsize>(tail.size()));
  };
  make(std::string(16, 'x'));
  REQUIRE(check_dump(image, "GGPE01", {}, nullptr, &check, &error));
  CHECK(check.verdict == DumpVerdict::Unknown);
  CHECK(check.note.find("not padding") != std::string::npos);

  make(std::string("\xB9\x91\xFB\xC2\xD5\xB9\x02\x44\xB9\x91\xFB\xC2\xD5\xB9\x02\x44", 16));
  std::atomic<bool> cancel{true};
  CHECK_FALSE(check_dump(image, "GGPE01", {}, &cancel, &check, &error));
  CHECK(error == "cancelled");
  double last = 0;
  REQUIRE(check_dump(image, "GGPE01", [&](double f) { last = f; }, nullptr, &check, &error));
  CHECK(last == doctest::Approx(1.0));
  CHECK(check.verdict == DumpVerdict::Unknown);  // padded well, but all zeros
  CHECK(check.dump == "Mario Kart Arcade GP [Feb 14 2006 13:09:48]");
  CHECK(check.body_sha1.size() == 40);
  CHECK(check.note.find("differs") != std::string::npos);

  std::int64_t size = 0, modified = 0;
  REQUIRE(image_stamp(image, &size, &modified));
  CHECK(size == static_cast<std::int64_t>(kGpSize + 16));
  CHECK_FALSE(image_stamp(utf8(dir.path / "none.iso"), &size, &modified));
}

TEST_CASE("a disc image of another release says which game it is, and is not recorded") {
  TempDir dir("opengp_launcher_add_image");
  const fs::path src = dir.path / "src";
  write(src / "games" / "mkgp2" / "game.toml", kGameToml);  // dol_sha1 "abc": no image is this release
  std::string image(0x440 + 0x100, '\0');
  image.replace(0, 6, "GGPE02");
  image.replace(0x1C, 4, "\xC2\x33\x9F\x3D");
  image.replace(0x420, 4, std::string("\0\0\x04\x40", 4));
  write(dir.path / "game.iso", image);
  write(dir.path / "junk.iso", "not a disc");
  Home home;
  std::string error, id;
  REQUIRE_MESSAGE(open_home(src, {}, &home, &error), error);

  // The launcher keeps the reason on that game's page.
  CHECK_FALSE(add_image(home, utf8(dir.path / "game.iso"), &id, &error));
  CHECK(id == "GGPE02");
  CHECK(error.find("not the release OpenGP supports") != std::string::npos);
  CHECK(app::remembered_image(home.user_dir, "GGPE02").empty());

  // Not a disc image at all: no game to say.
  id.clear();
  CHECK_FALSE(add_image(home, utf8(dir.path / "junk.iso"), &id, &error));
  CHECK(id.empty());
}

TEST_CASE("the player is told why a game stopped, and about its reports") {
  TempDir dir("opengp_launcher_end");
  const fs::path user = dir.path / "user";
  const fs::path log = user / "logs" / "GGPE02.log";
  const auto started = fs::file_time_type::clock::now();

  // A clean exit says nothing.
  write(log, "[opengp] video: vulkan\n[opengp] quit from the menu\n");
  GameEnd end = game_end("GP 2", 0, false, log, user, started);
  CHECK_FALSE(end.error);
  CHECK(end.text.empty());

  // Why it could not start: the last line that isn't a report's detail.
  write(log, "[opengp] booting\n  guest: draws 0.0 s\nNo disc image of GP 2 (GGPE02) found. Add it.\n");
  end = game_end("GP 2", 1, false, log, user, started);
  CHECK(end.error);
  CHECK(end.text == "GP 2 could not run. No disc image of GP 2 (GGPE02) found. Add it.");

  // A crash (OpenGP's exit code 3) names the report written during the run.
  write(user / "crash-report.txt", "report");
  write(log, "[crash] report written to somewhere\n");
  end = game_end("GP 2", kCrashExitCode, false, log, user, started);
  CHECK(end.error);
  CHECK(end.text.find("GP 2 crashed. What went wrong is in ") == 0);
  CHECK(end.text.find("crash-report.txt") != std::string::npos);
  // ... but not one from an earlier run.
  fs::last_write_time(user / "crash-report.txt", started - std::chrono::hours(1));
  CHECK(game_end("GP 2", kCrashExitCode, false, log, user, started).text == "GP 2 crashed.");

  // A hang report from this run, and a notice the game could no longer show.
  write(user / "hang-report.txt", "hang");
  write(log, "[opengp] something\n[unseen] The settings could not be saved: disk full\n");
  end = game_end("GP 2", 0, false, log, user, started);
  CHECK(end.error);
  CHECK(end.text.find("GP 2 stopped responding for a while.") == 0);
  CHECK(end.text.find("\n\nThe settings could not be saved: disk full") != std::string::npos);

  CHECK(last_reason("why\n[opengp] status: 10 frames\n  guest: x\n[crash] y\n") == "why");
}

TEST_CASE("the player is told why a disc image isn't used") {
  TempDir dir("opengp_launcher_images");
  const fs::path pkg = dir.path / "pkg";
  write(pkg / "sdk" / "package.toml", "version = \"9.9.9\"\ngames = [\"mkgp2\"]\n[build]\nexecutable_suffix = \"" +
                                          std::string(kExe) +
                                          "\"\ncommon_sources = [\"main.cpp\"]\ngame_sources = [\"patches.cpp\"]\n");
  write(pkg / "sdk" / "games" / "mkgp2" / "game.toml", kGameToml);
  write(pkg / "ISO" / "game.rvz", "compressed");
  write(pkg / "ISO" / "junk.iso", "not a disc");
  Home home;
  std::string error;
  REQUIRE_MESSAGE(open_home(pkg, {}, &home, &error), error);

  // The image it was installed from has gone.
  REQUIRE(app::remember_image(home.user_dir, "GGPE02", utf8(dir.path / "moved.iso"), &error));
  std::vector<std::string> skipped;
  const std::vector<GameStatus> status = check_games(home, &skipped);
  REQUIRE(status.size() == 1);
  CHECK(status[0].state == GameState::NoImage);
  CHECK(status[0].note.find("no longer at") != std::string::npos);
  REQUIRE(skipped.size() == 2);
  std::string all;
  for (const std::string& s : skipped) all += s + "\n";
  CHECK(all.find("game.rvz: a compressed image; convert it to .iso with Dolphin") != std::string::npos);
  CHECK(all.find("junk.iso: not a GameCube or Triforce disc image") != std::string::npos);

  // Chosen with Add disc image: the same reasons.
  std::string id;
  CHECK_FALSE(add_image(home, utf8(pkg / "ISO" / "game.rvz"), &id, &error));
  CHECK(error.find("A compressed image; convert it to .iso with Dolphin") == 0);
  CHECK_FALSE(add_image(home, utf8(dir.path / "nothere.iso"), &id, &error));
  CHECK(error == "No such file.");
}

TEST_CASE("with no OpenGP folder, no disc images are looked for") {
  // An app on macOS starts in /, whose private/ the launcher used to search
  // (stopping at the first folder in it that it could not read).
  TempDir dir("opengp_launcher_no_home");
  write(dir.path / "private" / "x.iso", "not a disc");
  write(dir.path / "ISO" / "y.iso", "not a disc");
  const fs::path was = fs::current_path();
  fs::current_path(dir.path);
  Home home;
  std::string error;
  CHECK_FALSE(open_home(dir.path / "not-opengp", {}, &home, &error));
  std::vector<std::string> skipped;
  CHECK(check_games(home, &skipped).empty());
  CHECK(skipped.empty());
  fs::current_path(was);
}

#ifndef _WIN32
TEST_CASE("a folder the player can't read is passed over, and a folder they can't write to is named") {
  TempDir dir("opengp_launcher_unreadable");
  const fs::path pkg = dir.path / "pkg";
  write(pkg / "sdk" / "package.toml", "version = \"9.9.9\"\ngames = [\"mkgp2\"]\n[build]\nexecutable_suffix = \"" +
                                          std::string(kExe) +
                                          "\"\ncommon_sources = [\"main.cpp\"]\ngame_sources = [\"patches.cpp\"]\n");
  write(pkg / "sdk" / "games" / "mkgp2" / "game.toml", kGameToml);
  write(pkg / "ISO" / "locked" / "a.iso", "not a disc");
  write(pkg / "ISO" / "junk.iso", "not a disc");
  Home home;
  std::string error;
  REQUIRE_MESSAGE(open_home(pkg, {}, &home, &error), error);
  CHECK(check_writable(pkg, &error));

  fs::permissions(pkg / "ISO" / "locked", fs::perms::none);
  std::vector<std::string> skipped;
  CHECK(check_games(home, &skipped).size() == 1);
  CHECK(skipped == std::vector<std::string>{"junk.iso: not a GameCube or Triforce disc image; skipped"});
  fs::permissions(pkg / "ISO" / "locked", fs::perms::owner_all);

  if (::geteuid() != 0) {  // root can write anywhere
    fs::remove_all(pkg / "build");
    fs::permissions(pkg, fs::perms::owner_read | fs::perms::owner_exec);
    CHECK_FALSE(check_writable(pkg, &error));
    CHECK(error.find("OpenGP cannot write to its folder, ") == 0);
    CHECK(error.find("). Move the OpenGP folder somewhere you can write to") != std::string::npos);
    fs::permissions(pkg, fs::perms::owner_all);
  }
}
#endif
