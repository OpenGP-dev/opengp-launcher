// SPDX-License-Identifier: GPL-3.0-or-later
//
// Running a game: without a console window, its output going to a log file,
// and knowing when it has ended. A game restarts itself for its test menu
// (OpenGP's runtime/os/reboot.cpp): on Windows that is a new process (the old
// one exits), so the launcher follows every process the game starts, in a
// job object, and the game has ended when they all have. Elsewhere the
// restart keeps the process (exec).
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace launcher {

class GameProcess {
 public:
  GameProcess();
  ~GameProcess();  // does not stop the game
  GameProcess(const GameProcess&) = delete;
  GameProcess& operator=(const GameProcess&) = delete;

  // Starts `exe` (UTF-8) with `args`; its output (stdout and stderr) replaces
  // the file `log`.
  bool start(const std::string& exe, const std::vector<std::string>& args, const std::string& log,
             std::string* error);

  struct Result {
    int exit_code = 0;     // the last process's
    bool crashed = false;  // ended by a crash (or killed)
  };
  // Waits until the game, and any process it restarted as, has ended. Call
  // once, after a successful start (from any thread).
  Result wait();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Whether the game is running from the user folder `user_dir`, however it was
// started (its own shortcut, another launcher): an OpenGP 1.0.1 game holds
// user/.lock-<ID> while it runs (a 1.0.0 game holds none). Never creates the
// file.
bool game_running(const std::filesystem::path& user_dir, const std::string& game_id);

}  // namespace launcher
