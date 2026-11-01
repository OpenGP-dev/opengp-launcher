// SPDX-License-Identifier: GPL-3.0-or-later
//
// Updating an installed package to a newer release (qt/updater.h does the
// downloading). A release is one of the launcher's: its tag is the launcher's
// version, and its files are OpenGP's packages, named for OpenGP's version
// ("OpenGP-1.0.2-windows-x64.zip"). The new package is unpacked into
// <package>/update/new/, and its files take the place of the old ones, which
// go to <package>/update/old/ until the new launcher has started.
#pragma once

#include <string>

#include "core/home.h"

namespace launcher {

// The end of the name of this computer's package: "-windows-x64.zip",
// "-macos-arm64.tar.gz" or "-linux-x64.tar.gz".
const char* package_suffix();

// The OpenGP version in a package's name: "1.0.2" from
// "OpenGP-1.0.2-windows-x64.zip" (with `suffix` that of the package for this
// computer), or "" for any other name.
std::string package_version(const std::string& name, const std::string& suffix = package_suffix());

// Whether a release is an update: its launcher or its OpenGP is newer than
// the package's, and neither is older. `opengp` is "" for a release with no
// package for this computer (only its launcher's version is known then). Any
// other version that isn't one (release_version) is no update.
bool is_update(const std::string& launcher, const std::string& opengp, const std::string& current_launcher,
               const std::string& current_opengp);

// The SHA-256 in a file's digest on GitHub ("sha256:" and 64 hex digits), in
// lower case, or "" if it is not one.
std::string sha256_digest(const std::string& digest);

// Where an update is downloaded and unpacked, and the old files kept.
fs::path update_dir(const fs::path& home);

// What a finished update leaves for the new launcher in update_dir: the
// launcher's and OpenGP's versions it replaced, a line each.
fs::path update_done_file(const fs::path& home);

// Deletes update_dir, except old/ when it still has files and the update
// didn't finish (no update_done_file): the old files of one that could not
// all be put back, which the player needs.
void clean_update(const fs::path& home);

// The package unpacked in `dir`: `dir` itself or the one folder in it that
// has sdk/package.toml, or "" if there is none.
fs::path find_unpacked_package(const fs::path& dir);

// The launcher in a package folder: OpenGP.exe, OpenGP.app's program or OpenGP.
fs::path launcher_program(const fs::path& package);

// Replaces the package in `home` with the unpacked one in `package`. Every
// file and link of the new package takes the place of the old one (none of
// user/, toolchain/, build/ or update/, the player's and the launcher's own),
// and in the new package's folders (but not ISO/, where the player's images
// are) the old files it no longer has are taken out. Nothing is deleted:
// whatever is replaced or taken out is moved to `old`. The files are moved,
// not copied, so the running launcher's own can be replaced on Windows too.
// If a move fails, the ones before it are undone and `error` says why.
// `fail_after` (for the tests) makes the move after that many fail.
bool apply_update(const fs::path& home, const fs::path& package, const fs::path& old, std::string* error,
                  int fail_after = -1);

}  // namespace launcher
