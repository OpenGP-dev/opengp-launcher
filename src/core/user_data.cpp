// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/user_data.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <string_view>

#include "core/home.h"
#include "os/atomic_file.h"
#include "triforce/card_folder.h"

namespace launcher {

namespace {

fs::path cards_dir(const fs::path& user_dir, const std::string& game_id) { return user_dir / "cards" / game_id; }
fs::path card_file(const fs::path& user_dir, const std::string& game_id, const std::string& name) {
  return cards_dir(user_dir, game_id) / os::utf8_path(name + ".bin");
}

bool fail(std::string* error, const std::string& msg) {
  if (error) *error = msg;
  return false;
}

// What in user/ a backup leaves out (and a restore leaves alone): these
// folders, and the lock file each running game holds (.lock-<game ID>).
bool left_out(const fs::path& name) {
  return name == "backups" || name == "logs" || name == "art" || name == "screenshots" ||
         name.u8string().rfind(u8".lock-", 0) == 0;
}

std::string now_text() {
  const std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  char text[32];
  std::strftime(text, sizeof(text), "%Y-%m-%d %H-%M-%S", &tm);
  return text;
}

std::uintmax_t folder_size(const fs::path& dir) {
  std::uintmax_t total = 0;
  std::error_code ec;
  for (const auto& e : fs::recursive_directory_iterator(dir, ec))
    if (e.is_regular_file(ec)) total += e.file_size(ec);
  return total;
}

// Copies what a backup keeps of `from` (a user folder) into `to`.
bool copy_user_files(const fs::path& from, const fs::path& to, std::string* error) {
  std::error_code ec;
  fs::create_directories(to, ec);
  for (const auto& entry : fs::directory_iterator(from, ec)) {
    if (left_out(entry.path().filename())) continue;
    fs::copy(entry.path(), to / entry.path().filename(),
             fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) return fail(error, "cannot copy " + utf8(entry.path()) + ": " + ec.message());
  }
  return !ec || fail(error, "cannot read " + utf8(from) + ": " + ec.message());
}

}  // namespace

bool valid_card_name(const std::string& name) {
  if (name.empty() || name.size() > 64 || name.front() == ' ' || name.back() == ' ' || name.back() == '.') return false;
  // The games hold no card whose name OpenGP refuses: a leading dot, or
  // Windows' device names (CON, COM1...), with or without an extension.
  if (name.front() == '.') return false;
  std::string base = name.substr(0, name.find('.'));
  while (!base.empty() && base.back() == ' ') base.pop_back();
  for (char& c : base) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  for (const char* device : {"CON", "PRN", "AUX", "NUL"})
    if (base == device) return false;
  if (base.size() == 4 && (base.rfind("COM", 0) == 0 || base.rfind("LPT", 0) == 0) && base[3] >= '0' && base[3] <= '9')
    return false;
  return std::none_of(name.begin(), name.end(), [](char c) {
    return static_cast<unsigned char>(c) < 32 || std::string_view("\\/:*?\"<>|").find(c) != std::string_view::npos;
  });
}

std::vector<std::string> list_cards(const fs::path& user_dir, const std::string& game_id) {
  return triforce::CardFolder(utf8(cards_dir(user_dir, game_id))).list();
}

bool rename_card(const fs::path& user_dir, const std::string& game_id, const std::string& from, const std::string& to,
                 std::string* error) {
  if (!valid_card_name(to)) return fail(error, "\"" + to + "\" Invalid card name.");
  const fs::path target = card_file(user_dir, game_id, to);
  std::error_code ec;
  if (fs::exists(target, ec)) return fail(error, "There is already a card called \"" + to + "\".");
  fs::rename(card_file(user_dir, game_id, from), target, ec);
  return !ec || fail(error, "Failed to rename the card: " + ec.message());
}

std::string duplicate_card(const fs::path& user_dir, const std::string& game_id, const std::string& name,
                           std::string* error) {
  std::string copy = name + " (copy)";
  std::error_code ec;
  for (int n = 2; fs::exists(card_file(user_dir, game_id, copy), ec); ++n)
    copy = name + " (copy " + std::to_string(n) + ")";
  fs::copy_file(card_file(user_dir, game_id, name), card_file(user_dir, game_id, copy), ec);
  if (ec) {
    fail(error, "Failed to copy card: " + ec.message());
    return {};
  }
  return copy;
}

bool delete_card(const fs::path& user_dir, const std::string& game_id, const std::string& name, std::string* error) {
  std::error_code ec;
  fs::remove(card_file(user_dir, game_id, name), ec);
  return !ec || fail(error, "Failed to delete card: " + ec.message());
}

std::string import_card(const fs::path& user_dir, const std::string& game_id, const fs::path& file,
                        std::string* error) {
  std::error_code ec;
  const auto size = fs::file_size(file, ec);
  // A card holds 69 bytes; anything much bigger is not one.
  if (ec || size == 0 || size > 4096) {
    fail(error, utf8(file) + " is not a valid Mario Card file.");
    return {};
  }
  std::string base = utf8(file.stem());
  if (!valid_card_name(base)) base = "Card";
  std::string name = base;
  for (int n = 2; fs::exists(card_file(user_dir, game_id, name), ec); ++n) name = base + " " + std::to_string(n);
  fs::create_directories(cards_dir(user_dir, game_id), ec);
  fs::copy_file(file, card_file(user_dir, game_id, name), ec);
  if (ec) {
    fail(error, "Couldn't add the card: " + ec.message());
    return {};
  }
  return name;
}

bool export_card(const fs::path& user_dir, const std::string& game_id, const std::string& name, const fs::path& file,
                 std::string* error) {
  std::error_code ec;
  fs::copy_file(card_file(user_dir, game_id, name), file, fs::copy_options::overwrite_existing, ec);
  return !ec || fail(error, "Failed to save card: " + ec.message());
}

std::vector<Backup> list_backups(const fs::path& user_dir) {
  std::vector<Backup> out;
  std::error_code ec;
  for (const auto& entry : fs::directory_iterator(user_dir / "backups", ec))
    if (entry.is_directory(ec)) out.push_back({utf8(entry.path().filename()), folder_size(entry.path())});
  // Names start with the date and time, so the newest sort last.
  std::sort(out.begin(), out.end(), [](const Backup& a, const Backup& b) { return a.name > b.name; });
  return out;
}

std::string back_up(const fs::path& user_dir, const std::string& note, std::string* error) {
  std::string name = now_text() + (note.empty() ? "" : " " + note);
  std::error_code ec;
  for (int n = 2; fs::exists(user_dir / "backups" / os::utf8_path(name), ec); ++n)
    name = now_text() + " (" + std::to_string(n) + ")" + (note.empty() ? "" : " " + note);
  if (!copy_user_files(user_dir, user_dir / "backups" / os::utf8_path(name), error)) return {};
  return name;
}

bool restore_backup(const fs::path& user_dir, const std::string& name, std::string* error) {
  const fs::path backup = user_dir / "backups" / os::utf8_path(name);
  std::error_code ec;
  if (!fs::is_directory(backup, ec)) return fail(error, "There is no backup called \"" + name + "\".");
  if (back_up(user_dir, "before restoring", error).empty()) return false;
  // Clear what a backup holds, then copy the backup in.
  for (const auto& entry : fs::directory_iterator(user_dir, ec)) {
    if (left_out(entry.path().filename())) continue;
    fs::remove_all(entry.path(), ec);
    if (ec) return fail(error, "Cannot remove " + utf8(entry.path()) + ": " + ec.message());
  }
  return copy_user_files(backup, user_dir, error);
}

bool delete_backup(const fs::path& user_dir, const std::string& name, std::string* error) {
  std::error_code ec;
  if (name.empty()) return fail(error, "No backup selected");
  fs::remove_all(user_dir / "backups" / os::utf8_path(name), ec);
  return !ec || fail(error, "Failed to delete backup: " + ec.message());
}

std::vector<fs::path> game_data_files(const fs::path& user_dir, const std::string& game_id) {
  std::vector<fs::path> out;
  std::error_code ec;
  for (const char* prefix : {"tribackup_", "backup_", "tridimm_"}) {
    const fs::path file = user_dir / (prefix + game_id + ".bin");
    if (fs::is_regular_file(file, ec)) out.push_back(file);
  }
  return out;
}

bool reset_game_data(const fs::path& user_dir, const std::string& game_id, std::string* error) {
  const auto files = game_data_files(user_dir, game_id);
  if (files.empty()) return true;
  if (back_up(user_dir, "before resetting " + game_id, error).empty()) return false;
  std::error_code ec;
  for (const fs::path& f : files) {
    fs::remove(f, ec);
    if (ec) return fail(error, "Cannot delete " + utf8(f) + ": " + ec.message());
  }
  return true;
}

std::vector<fs::path> list_screenshots(const fs::path& user_dir) {
  std::vector<std::pair<fs::file_time_type, fs::path>> shots;
  std::error_code ec;
  for (const auto& entry : fs::directory_iterator(user_dir / "screenshots", ec))
    if (entry.path().extension() == ".png") shots.emplace_back(entry.last_write_time(ec), entry.path());
  std::sort(shots.begin(), shots.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
  std::vector<fs::path> out;
  for (auto& [time, path] : shots) out.push_back(std::move(path));
  return out;
}

fs::path log_path(const fs::path& user_dir, const std::string& game_id) {
  return user_dir / "logs" / (game_id + ".log");
}

fs::path next_log(const fs::path& user_dir, const std::string& game_id) {
  const fs::path log = log_path(user_dir, game_id);
  std::error_code ec;
  fs::create_directories(log.parent_path(), ec);
  if (fs::exists(log, ec)) fs::rename(log, user_dir / "logs" / (game_id + ".1.log"), ec);
  return log;
}

}  // namespace launcher
