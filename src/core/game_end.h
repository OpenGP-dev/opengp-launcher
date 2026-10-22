// SPDX-License-Identifier: GPL-3.0-or-later
//
// What to tell the player when a game the launcher started has ended: why it
// could not start, that it crashed or stopped responding (and where the
// report is), and messages the game could no longer show in its window.
#pragma once

#include <filesystem>
#include <string>

namespace launcher {

namespace fs = std::filesystem;

// OpenGP's exit code after a crash (it writes user/crash-report.txt first).
inline constexpr int kCrashExitCode = 3;

struct GameEnd {
  bool error = false;  // something went wrong: shown as an error, with the window brought forward
  std::string text;    // "" when there is nothing to say
};

// `title`: the game's name. `log`: the run's output (stdout and stderr).
// `user_dir`: where the game writes its reports. `started`: when the run
// began; reports older than that are from earlier runs.
GameEnd game_end(const std::string& title, int exit_code, bool crashed, const fs::path& log, const fs::path& user_dir,
                 fs::file_time_type started);

// The line of a game's output that says why it stopped: the last one that is
// not part of a status report, a backtrace or the launcher's own notes.
std::string last_reason(const std::string& output);

}  // namespace launcher
