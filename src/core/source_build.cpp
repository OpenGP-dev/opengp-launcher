// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/source_build.h"

#include <cstdio>

#include "disc/disc.h"
#include "disc/sha1.h"
#include "os/atomic_file.h"

namespace launcher {

const char* default_preset() {
#ifdef _WIN32
  return "clang-cl";
#else
  return "default";
#endif
}

bool extract_dol(const Home& home, const Game& game, const std::string& image, std::string* error) {
  auto fail = [&](const std::string& msg) {
    if (error) *error = msg;
    return false;
  };
  disc::Image img;
  std::string err;
  if (!img.open(image, &err)) return fail(err);
  std::vector<gekko::u8> dol;
  if (!img.read_dol(&dol)) return fail(image + ": cannot read its main.dol");
  const std::string sha1 = disc::to_hex(disc::sha1(dol.data(), dol.size()));
  if (sha1 != game.info.dol_sha1)
    return fail(image + ": its main.dol (SHA-1 " + sha1 + ") is not the supported release of " + game.info.title +
                " (SHA-1 " + game.info.dol_sha1 + ")");
  const fs::path out = home.dir / "orig" / game.info.id / "sys" / "main.dol";
  std::error_code ec;
  fs::create_directories(out.parent_path(), ec);
  if (!os::write_file_atomically(utf8(out), dol.data(), dol.size())) return fail("cannot write " + utf8(out));
  return true;
}

SourceBuild source_build(const Home& home, const Game& game, const std::string& cmake, const std::string& ninja) {
  SourceBuild b;
  // Game::exe is build/<dir>/games/<game>/<executable>.
  b.build_dir = game.exe.parent_path().parent_path().parent_path();
  std::error_code ec;
  if (fs::is_regular_file(b.build_dir / "CMakeCache.txt", ec)) {
    // Configured before (by a preset or by hand): again, now with the game's main.dol.
    b.commands.push_back({cmake, "-S", utf8(home.dir), "-B", utf8(b.build_dir), "-DOPENGP_BUILD_GAMES=ON"});
  } else {
    std::vector<std::string> configure = {cmake, "--preset", utf8(b.build_dir.filename()), "-DOPENGP_BUILD_GAMES=ON"};
    if (!ninja.empty()) configure.push_back("-DCMAKE_MAKE_PROGRAM=" + ninja);
    b.commands.push_back(std::move(configure));
  }
  b.commands.push_back({cmake, "--build", utf8(b.build_dir), "--target", game.info.executable});
  return b;
}

double ninja_progress(const std::string& line) {
  unsigned done = 0, total = 0;
  char close = 0;
  if (std::sscanf(line.c_str(), "[%u/%u%c", &done, &total, &close) != 3 || close != ']' || total == 0) return -1;
  return static_cast<double>(done) / total;
}

}  // namespace launcher
