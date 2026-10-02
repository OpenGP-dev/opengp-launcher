// SPDX-License-Identifier: GPL-3.0-or-later
//
// Checking a disc image against the known good dumps of the games, as FIX94's
// triforce-header-patcher does: by the SHA-1 of the image's body, everything
// after its 0x40-byte header (which converters and patchers rewrite: the
// game's ID and title), up to the dump's size. Some dumps of GP are longer,
// padded with a fill pattern; they are good. One known bad dump of GP 2 has a
// damaged texture: the patcher repairs it (the launcher cannot, as that needs
// the game's own data).
//
// Adding an image only checks its main.dol (library.h), which is enough to
// build the game; this checks the rest of it too (the courses, the sounds).
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace launcher {

enum class DumpVerdict {
  Good,      // a known good dump
  KnownBad,  // a known bad dump: `note` says what is wrong
  Unknown,   // not a dump the launcher knows: `note` says how it differs
};

struct DumpCheck {
  DumpVerdict verdict = DumpVerdict::Unknown;
  std::string dump;       // the known good dump it is, or should be ("Mario Kart Arcade GP 2 [Feb 7 2007 02:47:24]")
  std::string note;       // more about it, for the player ("" if nothing)
  std::string body_sha1;  // what it hashed to ("" if it was not hashed)
};

// Checks `image` (UTF-8 path), which add_image has found to be the game
// `game_id`. Reports the fraction done; stops with the error "cancelled" when
// `cancel` is set. Fails if the image cannot be read or the game has no known
// dump.
bool check_dump(const std::string& image, const std::string& game_id, const std::function<void(double)>& progress,
                const std::atomic<bool>* cancel, DumpCheck* out, std::string* error);

// The image's size and modification time now, to tell whether an ImageCheck
// (prefs.h) of it still holds. False if it is not there.
bool image_stamp(const std::string& image, std::int64_t* size, std::int64_t* modified);

// SHA-1 over data given in pieces (OpenGP's disc::sha1 takes it whole).
class Sha1 {
 public:
  Sha1();
  void update(const void* data, std::size_t size);
  std::string hex();  // the digest, in lowercase hex; call once, after the data

 private:
  std::uint32_t h_[5];
  std::uint8_t block_[64];
  std::size_t used_ = 0;
  std::uint64_t length_ = 0;
};

}  // namespace launcher
