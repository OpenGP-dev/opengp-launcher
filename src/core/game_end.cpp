// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/game_end.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>

#include "core/home.h"  // utf8

namespace launcher {

namespace {

// The end of the log: what a game prints last is what matters, and a long
// session's log can be large.
std::string read_tail(const fs::path& path, std::size_t max_bytes = 256 * 1024) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return {};
  f.seekg(0, std::ios::end);
  const auto size = static_cast<std::size_t>(std::max<std::streamoff>(0, f.tellg()));
  const std::size_t start = size > max_bytes ? size - max_bytes : 0;
  f.seekg(static_cast<std::streamoff>(start));
  std::string text(size - start, '\0');
  f.read(text.data(), static_cast<std::streamsize>(text.size()));
  text.resize(static_cast<std::size_t>(f.gcount()));
  return text;
}

std::vector<std::string> lines_of(const std::string& text) {
  std::vector<std::string> out;
  std::istringstream in(text);
  for (std::string line; std::getline(in, line);) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    out.push_back(line);
  }
  return out;
}

bool starts_with(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

// A report written during this run, if there is one.
std::string new_report(const fs::path& path, fs::file_time_type started) {
  std::error_code ec;
  const auto written = fs::last_write_time(path, ec);
  // Some file systems keep times to 2 seconds.
  if (ec || written + std::chrono::seconds(2) < started) return {};
  return utf8(path);
}

}  // namespace

std::string last_reason(const std::string& output) {
  const std::vector<std::string> lines = lines_of(output);
  for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
    const std::string& line = *it;
    if (line.empty() || line[0] == ' ' || line[0] == '\t') continue;  // details of a status report or a backtrace
    if (starts_with(line, "[opengp] status") || starts_with(line, "[opengp] timeout") ||
        starts_with(line, "[unseen]") || starts_with(line, "[crash]"))
      continue;
    return line;
  }
  return {};
}

GameEnd game_end(const std::string& title, int exit_code, bool crashed, const fs::path& log, const fs::path& user_dir,
                 fs::file_time_type started) {
  const std::string output = read_tail(log);
  GameEnd end;
  std::string text;
  auto add = [&text](const std::string& s) {
    if (s.empty()) return;
    if (!text.empty()) text += "\n\n";
    text += s;
  };

  const bool crash = crashed || exit_code == kCrashExitCode;
  if (crash) {
    const std::string report = new_report(user_dir / "crash-report.txt", started);
    add(title + " crashed." +
        (report.empty() ? std::string()
                        : " What went wrong is in " + report + ": please attach it to a bug report."));
  } else if (exit_code != 0) {
    const std::string why = last_reason(output);
    add(title + " could not run." + (why.empty() ? std::string() : " " + why));
  }
  if (!crash)
    if (const std::string report = new_report(user_dir / "hang-report.txt", started); !report.empty())
      add(title + " stopped responding for a while. What it was doing is in " + report +
          ": please attach it to a bug report.");
  // What the game could no longer show in its window (a save that failed as
  // it closed).
  for (const std::string& line : lines_of(output))
    if (starts_with(line, "[unseen] ")) add(line.substr(9));

  end.error = !text.empty();
  end.text = std::move(text);
  return end;
}

}  // namespace launcher
