// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/update.h"

#include <algorithm>
#include <set>
#include <vector>

#include "core/news.h"

namespace launcher {

namespace {

// Folders of the package's that an update never touches: the player's
// settings and saves, the compiler the installer downloaded, the games'
// builds, and the update itself.
bool kept(const fs::path& relative) {
  const fs::path top = *relative.begin();
  return top == "user" || top == "toolchain" || top == "build" || top == "update";
}

// A file, or a link (to a folder too: a link is moved, never followed).
bool is_leaf(const fs::path& path) {
  std::error_code ec;
  const fs::file_status s = fs::symlink_status(path, ec);
  return !ec && fs::exists(s) && !fs::is_directory(s);
}

bool exists_here(const fs::path& path) {
  std::error_code ec;
  return fs::exists(fs::symlink_status(path, ec));
}

// The files and links under `root`, relative to it.
bool leaves(const fs::path& root, std::vector<fs::path>* out, std::error_code& ec) {
  for (auto it = fs::recursive_directory_iterator(root, ec); !ec && it != fs::recursive_directory_iterator();
       it.increment(ec))
    if (is_leaf(it->path())) out->push_back(it->path().lexically_relative(root));
  return !ec;
}

}  // namespace

const char* package_suffix() {
#if defined(_WIN32)
  return "-windows-x64.zip";
#elif defined(__APPLE__)
  return "-macos-arm64.tar.gz";
#else
  return "-linux-x64.tar.gz";
#endif
}

std::string package_version(const std::string& name, const std::string& suffix) {
  const std::string prefix = "OpenGP-";
  if (name.size() <= prefix.size() + suffix.size() || name.rfind(prefix, 0) != 0 ||
      name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0)
    return {};
  const std::string version = name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
  // Three numbers: not "v1.0.2", which release_version would take.
  return version[0] >= '0' && version[0] <= '9' ? release_version(version) : std::string();
}

bool is_update(const std::string& launcher, const std::string& opengp, const std::string& current_launcher,
               const std::string& current_opengp) {
  for (const std::string* version : {&launcher, &current_launcher, &current_opengp})
    if (release_version(*version).empty()) return false;
  const bool newer_launcher = newer_version(launcher, current_launcher);
  const bool newer_opengp = !opengp.empty() && newer_version(opengp, current_opengp);
  const bool older = newer_version(current_launcher, launcher) || (!opengp.empty() && newer_version(current_opengp, opengp));
  return (newer_launcher || newer_opengp) && !older;
}

std::string sha256_digest(const std::string& digest) {
  const std::string prefix = "sha256:";
  if (digest.size() != prefix.size() + 64 || digest.rfind(prefix, 0) != 0) return {};
  std::string hex = digest.substr(prefix.size());
  for (char& c : hex) {
    if (c >= 'A' && c <= 'F') c = static_cast<char>(c - 'A' + 'a');
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return {};
  }
  return hex;
}

fs::path update_dir(const fs::path& home) { return home / "update"; }

fs::path update_done_file(const fs::path& home) { return update_dir(home) / "updated-from.txt"; }

void clean_update(const fs::path& home) {
  const fs::path dir = update_dir(home);
  std::error_code ec;
  std::vector<fs::path> old_files;
  if (!fs::exists(update_done_file(home), ec) && fs::exists(dir / "old", ec) &&
      (!leaves(dir / "old", &old_files, ec) || !old_files.empty())) {
    for (const auto& entry : fs::directory_iterator(dir, ec))
      if (entry.path().filename() != "old") fs::remove_all(entry.path(), ec);
    return;
  }
  fs::remove_all(dir, ec);
}

fs::path find_unpacked_package(const fs::path& dir) {
  std::error_code ec;
  if (fs::is_regular_file(dir / "sdk" / "package.toml", ec)) return dir;
  fs::path found;
  for (const auto& entry : fs::directory_iterator(dir, ec)) {
    std::error_code e;
    if (!entry.is_directory(e) || !fs::is_regular_file(entry.path() / "sdk" / "package.toml", e)) continue;
    if (!found.empty()) return {};  // which one?
    found = entry.path();
  }
  return found;
}

fs::path launcher_program(const fs::path& package) {
#if defined(_WIN32)
  return package / "OpenGP.exe";
#elif defined(__APPLE__)
  return package / "OpenGP.app" / "Contents" / "MacOS" / "OpenGP";
#else
  return package / "OpenGP";
#endif
}

bool apply_update(const fs::path& home, const fs::path& package, const fs::path& old, std::string* error,
                  int fail_after) {
  struct Move {
    fs::path from, to;
  };
  std::vector<Move> done;
  std::string why;
  auto move = [&](const fs::path& from, const fs::path& to) {
    std::error_code ec;
    if (fail_after >= 0 && static_cast<int>(done.size()) >= fail_after) ec = std::make_error_code(std::errc::io_error);
    if (!ec) fs::create_directories(to.parent_path(), ec);
    if (!ec) fs::rename(from, to, ec);
    if (ec) {
      why = "could not move " + utf8(from) + " to " + utf8(to) + ": " + ec.message();
      return false;
    }
    done.push_back({from, to});
    return true;
  };
  auto fail = [&] {
    // Back as they were, the last move first.
    std::string not_undone;
    for (auto it = done.rbegin(); it != done.rend(); ++it) {
      std::error_code ec;
      fs::create_directories(it->from.parent_path(), ec);  // an empty folder taken out on the way
      fs::rename(it->to, it->from, ec);
      if (ec && not_undone.empty()) not_undone = utf8(it->from) + " (" + ec.message() + ")";
    }
    if (error)
      *error = not_undone.empty() ? why
                                  : why + "; and could not put back " + not_undone + ": the old files are in " + utf8(old);
    return false;
  };

  // The new package's files, and the folders at its top.
  std::vector<fs::path> files;
  std::error_code ec;
  if (!leaves(package, &files, ec)) {
    why = "could not read " + utf8(package) + ": " + ec.message();
    return fail();
  }
  files.erase(std::remove_if(files.begin(), files.end(), kept), files.end());
  if (files.empty()) {
    why = utf8(package) + " is empty";
    return fail();
  }
  std::set<fs::path> new_files(files.begin(), files.end());
  std::vector<fs::path> folders;
  for (const auto& entry : fs::directory_iterator(package, ec)) {
    const fs::path name = entry.path().filename();
    std::error_code e;
    if (!entry.is_symlink(e) && entry.is_directory(e) && !kept(name) && name != "ISO") folders.push_back(name);
  }

  // The old files in those folders that the new package doesn't have.
  for (const fs::path& folder : folders) {
    std::error_code e;
    if (!fs::is_directory(fs::symlink_status(home / folder, e))) continue;  // a file there: replaced below
    std::vector<fs::path> old_files;
    if (!leaves(home / folder, &old_files, e)) {
      why = "could not read " + utf8(home / folder) + ": " + e.message();
      return fail();
    }
    for (const fs::path& f : old_files) {
      const fs::path relative = folder / f;
      if (!new_files.count(relative) && !move(home / relative, old / relative)) return fail();
    }
  }

  for (const fs::path& relative : files) {
    // Each folder on the way must be a folder: a file or a link in the way
    // goes too (a link to a folder is not followed into the old package).
    fs::path partial;
    for (const fs::path& part : relative.parent_path()) {
      partial /= part;
      if (is_leaf(home / partial) && !move(home / partial, old / partial)) return fail();
    }
    // A folder where the new package has a file: emptied of the old files
    // above, so only empty folders are left in it, which go; one with files
    // in it (in ISO/, the player's) is moved.
    std::vector<fs::path> inside;
    std::error_code e;
    if (fs::is_directory(fs::symlink_status(home / relative, e)) && leaves(home / relative, &inside, e) &&
        inside.empty())
      fs::remove_all(home / relative, e);
    if (exists_here(home / relative) && !move(home / relative, old / relative)) return fail();
    if (!move(package / relative, home / relative)) return fail();
  }
  return true;
}

}  // namespace launcher
