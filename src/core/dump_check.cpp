// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/dump_check.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "os/atomic_file.h"

namespace launcher {

namespace {

namespace fs = std::filesystem;

constexpr std::uint64_t kHeaderSize = 0x40;  // the ID and title, which converters and patchers rewrite

// A region that is known to be wrong in some dumps.
struct KnownError {
  std::uint64_t offset, size;
  const char* sha1;  // of the region as those dumps have it
  const char* note;
};

struct KnownDump {
  const char* game_id;
  const char* name;
  std::uint64_t size;
  const char* body_sha1;   // of [kHeaderSize, size)
  const char* padding;     // the fill pattern some longer dumps end with (8 bytes), or nullptr
  const KnownError* error;  // or nullptr
};

// From triforce-header-patcher (github.com/FIX94/triforce-header-patcher,
// staticVals.h and good_tex_gp2.h); GP 2's body SHA-1 checked against a dump.
const KnownError kGp2BadTexture = {
    0x1A6A0000, 0x3000, "1230ea4a308befa331aa646db2d7ba5d375c2b6d",
    "A texture is damaged, a known error in some dumps of GP 2. FIX94's triforce-header-patcher repairs it."};

const KnownDump kDumps[] = {
    {"GGPE02", "Mario Kart Arcade GP 2 [Feb 7 2007 02:47:24]", 0x1E000000, "9e1d69fb7ecc00c7c927d4b4d811e50b58509055",
     nullptr, &kGp2BadTexture},
    {"GGPE01", "Mario Kart Arcade GP [Feb 14 2006 13:09:48]", 0x15000000, "e6ea1c8314a7346616f74d69ec21ae40f12fb95d",
     "\xB9\x91\xFB\xC2\xD5\xB9\x02\x44", nullptr},
};

std::string megabytes(std::uint64_t bytes) { return std::to_string((bytes + (1 << 19)) >> 20) + " MB"; }

std::uint32_t rotl(std::uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

void sha1_block(std::uint32_t h[5], const std::uint8_t* p) {
  std::uint32_t w[80];
  for (int i = 0; i < 16; ++i)
    w[i] = std::uint32_t{p[4 * i]} << 24 | std::uint32_t{p[4 * i + 1]} << 16 | std::uint32_t{p[4 * i + 2]} << 8 | p[4 * i + 3];
  for (int i = 16; i < 80; ++i) w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
  std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
  for (int i = 0; i < 80; ++i) {
    std::uint32_t f, k;
    if (i < 20) f = (b & c) | (~b & d), k = 0x5A827999;
    else if (i < 40) f = b ^ c ^ d, k = 0x6ED9EBA1;
    else if (i < 60) f = (b & c) | (b & d) | (c & d), k = 0x8F1BBCDC;
    else f = b ^ c ^ d, k = 0xCA62C1D6;
    const std::uint32_t t = rotl(a, 5) + f + e + k + w[i];
    e = d, d = c, c = rotl(b, 30), b = a, a = t;
  }
  h[0] += a, h[1] += b, h[2] += c, h[3] += d, h[4] += e;
}

}  // namespace

bool image_stamp(const std::string& image, std::int64_t* size, std::int64_t* modified) {
  const fs::path path = os::utf8_path(image);
  std::error_code ec, ec2;
  const auto bytes = fs::file_size(path, ec);
  const auto time = fs::last_write_time(path, ec2);
  if (ec || ec2) return false;
  *size = static_cast<std::int64_t>(bytes);
  *modified = static_cast<std::int64_t>(time.time_since_epoch().count());
  return true;
}

Sha1::Sha1() : h_{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0} {}

void Sha1::update(const void* data, std::size_t size) {
  const auto* p = static_cast<const std::uint8_t*>(data);
  length_ += size;
  while (size) {
    const std::size_t n = std::min(size, sizeof(block_) - used_);
    std::memcpy(block_ + used_, p, n);
    used_ += n, p += n, size -= n;
    if (used_ == sizeof(block_)) {
      sha1_block(h_, block_);
      used_ = 0;
    }
  }
}

std::string Sha1::hex() {
  const std::uint64_t bits = length_ * 8;
  const std::uint8_t one = 0x80, zero = 0;
  update(&one, 1);
  while (used_ != 56) update(&zero, 1);
  std::uint8_t tail[8];
  for (int i = 0; i < 8; ++i) tail[i] = static_cast<std::uint8_t>(bits >> (56 - 8 * i));
  update(tail, 8);
  static const char digits[] = "0123456789abcdef";
  std::string out;
  for (std::uint32_t v : h_)
    for (int shift = 28; shift >= 0; shift -= 4) out += digits[(v >> shift) & 15];
  return out;
}

bool check_dump(const std::string& image, const std::string& game_id, const std::function<void(double)>& progress,
                const std::atomic<bool>* cancel, DumpCheck* out, std::string* error) {
  auto fail = [&](const std::string& msg) {
    if (error) *error = msg;
    return false;
  };
  const KnownDump* dump = nullptr;
  for (const KnownDump& d : kDumps)
    if (game_id == d.game_id) dump = &d;
  if (!dump) return fail("there is no known dump of " + game_id + " to check against");

  const fs::path path = os::utf8_path(image);
  std::error_code ec;
  const std::uint64_t size = fs::file_size(path, ec);
  if (ec) return fail(image + ": " + ec.message());
  std::ifstream f(path, std::ios::binary);
  if (!f) return fail(image + ": cannot open");
  auto read = [&](std::uint64_t offset, void* dst, std::size_t n) {
    f.seekg(static_cast<std::streamoff>(offset));
    return static_cast<bool>(f.read(static_cast<char*>(dst), static_cast<std::streamsize>(n)));
  };

  DumpCheck check;
  check.dump = dump->name;
  if (size < dump->size) {
    check.note = "It is " + megabytes(size) + ", smaller than the good dump (" + megabytes(dump->size) +
                 "): it may be cut short.";
    *out = check;
    return true;
  }
  std::vector<char> buf(std::size_t{16} << 20);
  // Anything past the dump's size must be the fill pattern some dumps end with.
  if (size > dump->size) {
    bool padding = dump->padding != nullptr && (size - dump->size) % 8 == 0;
    for (std::uint64_t at = dump->size; padding && at < size; at += buf.size()) {
      const std::size_t n = static_cast<std::size_t>(std::min<std::uint64_t>(buf.size(), size - at));
      if (!read(at, buf.data(), n)) return fail(image + ": cannot read it");
      for (std::size_t i = 0; padding && i < n; i += 8) padding = std::memcmp(buf.data() + i, dump->padding, 8) == 0;
    }
    if (!padding) {
      check.note = "It is " + megabytes(size) + ", larger than the good dump (" + megabytes(dump->size) +
                   "), and the rest is not padding.";
      *out = check;
      return true;
    }
    check.note = "It has " + megabytes(size - dump->size) + " of padding at the end (Still Valid)";
  }
  if (const KnownError* e = dump->error) {
    std::vector<char> region(static_cast<std::size_t>(e->size));
    if (!read(e->offset, region.data(), region.size())) return fail(image + ": cannot read it");
    Sha1 sha;
    sha.update(region.data(), region.size());
    if (sha.hex() == e->sha1) {
      check.verdict = DumpVerdict::KnownBad;
      check.note = e->note;
      *out = check;
      return true;
    }
  }

  Sha1 sha;
  for (std::uint64_t at = kHeaderSize; at < dump->size; at += buf.size()) {
    if (cancel && *cancel) return fail("cancelled");
    const std::size_t n = static_cast<std::size_t>(std::min<std::uint64_t>(buf.size(), dump->size - at));
    if (!read(at, buf.data(), n)) return fail(image + ": cannot read");
    sha.update(buf.data(), n);
    if (progress) progress(static_cast<double>(at + n) / dump->size);
  }
  check.body_sha1 = sha.hex();
  if (check.body_sha1 == dump->body_sha1) {
    check.verdict = DumpVerdict::Good;
  } else {
    check.note = "This image differs from a known good dump. Expect issues in game";
  }
  *out = check;
  return true;
}

}  // namespace launcher
