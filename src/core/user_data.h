// SPDX-License-Identifier: GPL-3.0-or-later
//
// The player's files in user/: Mario Cards, the games' own saved data,
// backups of it all, screenshots and the games' logs.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace launcher {

namespace fs = std::filesystem;

// ---- Mario Cards: user/cards/<ID>/<name>.bin ----

std::vector<std::string> list_cards(const fs::path& user_dir, const std::string& game_id);
bool rename_card(const fs::path& user_dir, const std::string& game_id, const std::string& from, const std::string& to,
                 std::string* error);
// Copies a card to a new name ("<name> (copy)"); returns the new name, or "" on failure.
std::string duplicate_card(const fs::path& user_dir, const std::string& game_id, const std::string& name,
                           std::string* error);
bool delete_card(const fs::path& user_dir, const std::string& game_id, const std::string& name, std::string* error);
// Adds a card file from elsewhere, named after it (made unique); returns its name, or "" on failure.
std::string import_card(const fs::path& user_dir, const std::string& game_id, const fs::path& file,
                        std::string* error);
bool export_card(const fs::path& user_dir, const std::string& game_id, const std::string& name, const fs::path& file,
                 std::string* error);
// Whether a name can be a card's file name.
bool valid_card_name(const std::string& name);

// ---- Backups: user/backups/<date time>/, a copy of user/ ----
// (without backups/, logs/, art/ and screenshots/, which can be made again or
// are the player's to keep)

struct Backup {
  std::string name;  // "2026-10-16 19-05-12", maybe with a note: "2026-10-16 19-05-12 before restoring"
  std::uintmax_t bytes = 0;
};
std::vector<Backup> list_backups(const fs::path& user_dir);  // newest first
// Returns the backup's name, or "" on failure. `note` is added to its name.
std::string back_up(const fs::path& user_dir, const std::string& note, std::string* error);
// Puts user/ back as it was in the backup, after backing up what is there now.
bool restore_backup(const fs::path& user_dir, const std::string& name, std::string* error);
bool delete_backup(const fs::path& user_dir, const std::string& name, std::string* error);

// ---- A game's own saved data: its operator settings, records and rankings ----
// (tribackup_<ID>.bin, backup_<ID>.bin, tridimm_<ID>.bin)

std::vector<fs::path> game_data_files(const fs::path& user_dir, const std::string& game_id);  // those that exist
// Deletes them (the game starts as new), after a backup.
bool reset_game_data(const fs::path& user_dir, const std::string& game_id, std::string* error);

// ---- Screenshots and logs ----

std::vector<fs::path> list_screenshots(const fs::path& user_dir);  // newest first
// The log a game's next run writes: user/logs/<ID>.log, the last one moved to <ID>.1.log.
fs::path next_log(const fs::path& user_dir, const std::string& game_id);
fs::path log_path(const fs::path& user_dir, const std::string& game_id);

}  // namespace launcher
