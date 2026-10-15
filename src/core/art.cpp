// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/art.h"

#include <algorithm>
#include <deque>

#include "gx/texture.h"

namespace launcher {

namespace {

constexpr std::uint32_t kTplMagic = 0x0020AF30;

std::uint32_t be32(const std::vector<std::uint8_t>& d, std::size_t at) {
  return (std::uint32_t{d[at]} << 24) | (std::uint32_t{d[at + 1]} << 16) | (std::uint32_t{d[at + 2]} << 8) | d[at + 3];
}
std::uint32_t be16(const std::vector<std::uint8_t>& d, std::size_t at) { return (std::uint32_t{d[at]} << 8) | d[at + 1]; }

std::uint32_t alpha(std::uint32_t pixel) { return pixel >> 24; }

}  // namespace

std::uint32_t wrapped_size(const std::vector<std::uint8_t>& data) {
  if (data.size() <= 10 || data[8] != 0x78 || data[4] || data[5] || data[6] || data[7]) return 0;
  return std::uint32_t{data[0]} | (std::uint32_t{data[1]} << 8) | (std::uint32_t{data[2]} << 16) |
         (std::uint32_t{data[3]} << 24);
}

bool decode_tpl(const std::vector<std::uint8_t>& tpl, std::vector<Picture>* pictures, std::string* error) {
  auto fail = [&](const std::string& msg) {
    if (error) *error = msg;
    return false;
  };
  if (tpl.size() < 12 || be32(tpl, 0) != kTplMagic) return fail("not a TPL file");
  const std::uint32_t count = be32(tpl, 4), table = be32(tpl, 8);
  if (count > 4096 || table + std::size_t{count} * 8 > tpl.size()) return fail("bad image table");
  std::vector<Picture> out;
  for (std::uint32_t n = 0; n < count; ++n) {
    const std::uint32_t image = be32(tpl, table + n * 8), palette = be32(tpl, table + n * 8 + 4);
    if (image + 12 > tpl.size()) return fail("bad image header");
    const std::uint32_t height = be16(tpl, image), width = be16(tpl, image + 2);
    const std::uint32_t format = be32(tpl, image + 4), data = be32(tpl, image + 8);
    if (!gx::is_valid_format(format) || width == 0 || height == 0 || width > 4096 || height > 4096)
      return fail("unsupported image");
    const auto fmt = static_cast<gx::TexFormat>(format);
    if (data + std::size_t{gx::texture_level_size(fmt, width, height)} > tpl.size()) return fail("image data cut short");

    // The palette, padded to every entry the format can index.
    std::vector<std::uint8_t> tlut;
    auto tlut_format = gx::TlutFormat::IA8;
    if (gx::is_paletted(fmt)) {
      if (!palette || palette + 12 > tpl.size()) return fail("palette missing");
      const std::uint32_t entries = be16(tpl, palette), pal_format = be32(tpl, palette + 4);
      const std::uint32_t pal_data = be32(tpl, palette + 8);
      if (pal_format > 2 || pal_data + std::size_t{entries} * 2 > tpl.size()) return fail("bad palette");
      tlut.assign(std::size_t{gx::palette_entries(fmt)} * 2, 0);
      std::copy_n(tpl.begin() + pal_data, std::min<std::size_t>(entries * 2, tlut.size()), tlut.begin());
      tlut_format = static_cast<gx::TlutFormat>(pal_format);
    }

    Picture p;
    p.width = width;
    p.height = height;
    p.pixels.resize(std::size_t{width} * height);
    const std::uint32_t bw = gx::block_width(fmt), bh = gx::block_height(fmt);
    std::vector<std::uint32_t> scratch(std::size_t{(width + bw - 1) / bw * bw} * ((height + bh - 1) / bh * bh));
    gx::decode_texture(p.pixels.data(), tpl.data() + data, fmt, width, height, tlut.empty() ? nullptr : tlut.data(),
                       tlut_format, scratch);
    out.push_back(std::move(p));
  }
  *pictures = std::move(out);
  return true;
}

std::vector<Picture> stitch_screens(const std::vector<Picture>& tiles) {
  // Runs of same-height tiles 640 wide in total make rows...
  struct Row {
    std::size_t first, end;
    Picture picture;
  };
  std::vector<Row> rows;
  for (std::size_t i = 0; i < tiles.size();) {
    std::size_t j = i;
    std::uint32_t width = 0;
    while (j < tiles.size() && tiles[j].height == tiles[i].height && width < 640) width += tiles[j++].width;
    if (width != 640 || j - i < 2) {
      ++i;
      continue;
    }
    Picture row;
    row.width = 640;
    row.height = tiles[i].height;
    row.pixels.resize(std::size_t{640} * row.height);
    std::uint32_t x0 = 0;
    for (std::size_t t = i; t < j; ++t) {
      for (std::uint32_t y = 0; y < row.height; ++y)
        std::copy_n(tiles[t].pixels.begin() + std::size_t{y} * tiles[t].width, tiles[t].width,
                    row.pixels.begin() + std::size_t{y} * 640 + x0);
      x0 += tiles[t].width;
    }
    rows.push_back({i, j, std::move(row)});
    i = j;
  }
  // ... and rows that follow each other, up to 512 lines, one picture.
  std::vector<Picture> screens;
  for (std::size_t k = 0; k < rows.size(); ++k) {
    Picture screen = rows[k].picture;
    while (k + 1 < rows.size() && rows[k + 1].first == rows[k].end &&
           screen.height + rows[k + 1].picture.height <= 512) {
      ++k;
      screen.pixels.insert(screen.pixels.end(), rows[k].picture.pixels.begin(), rows[k].picture.pixels.end());
      screen.height += rows[k].picture.height;
    }
    screens.push_back(std::move(screen));
  }
  return screens;
}

bool apply_mask(Picture* picture, const Picture& mask) {
  if (mask.width != picture->width || mask.height != picture->height) return false;
  for (std::size_t i = 0; i < picture->pixels.size(); ++i) {
    const std::uint32_t m = mask.pixels[i];
    const std::uint32_t grey = ((m & 0xFF) * 299 + ((m >> 8) & 0xFF) * 587 + ((m >> 16) & 0xFF) * 114) / 1000;
    const std::uint32_t a = alpha(picture->pixels[i]) * grey / 255;
    picture->pixels[i] = (picture->pixels[i] & 0x00FFFFFF) | (a << 24);
  }
  return true;
}

Picture crop(const Picture& picture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height) {
  Picture out;
  if (x >= picture.width || y >= picture.height) return out;
  out.width = std::min(width, picture.width - x);
  out.height = std::min(height, picture.height - y);
  out.pixels.resize(std::size_t{out.width} * out.height);
  for (std::uint32_t row = 0; row < out.height; ++row)
    std::copy_n(picture.pixels.begin() + std::size_t{y + row} * picture.width + x, out.width,
                out.pixels.begin() + std::size_t{row} * out.width);
  return out;
}

Picture trim(const Picture& picture) {
  std::uint32_t x0 = picture.width, y0 = picture.height, x1 = 0, y1 = 0;
  for (std::uint32_t y = 0; y < picture.height; ++y)
    for (std::uint32_t x = 0; x < picture.width; ++x)
      if (alpha(picture.at(x, y))) {
        x0 = std::min(x0, x), y0 = std::min(y0, y);
        x1 = std::max(x1, x + 1), y1 = std::max(y1, y + 1);
      }
  return x1 > x0 ? crop(picture, x0, y0, x1 - x0, y1 - y0) : Picture{};
}

void key_out_background(Picture* picture) {
  const std::uint32_t w = picture->width, h = picture->height;
  auto pale = [&](std::uint32_t x, std::uint32_t y) {
    const std::uint32_t p = picture->at(x, y);
    const std::uint32_t r = p & 0xFF, g = (p >> 8) & 0xFF, b = (p >> 16) & 0xFF;
    const std::uint32_t lo = std::min({r, g, b}), hi = std::max({r, g, b});
    return alpha(p) && lo >= 150 && hi - lo <= 60;
  };
  std::vector<bool> seen(std::size_t{w} * h);
  std::deque<std::pair<std::uint32_t, std::uint32_t>> queue;
  auto visit = [&](std::uint32_t x, std::uint32_t y) {
    if (!seen[std::size_t{y} * w + x] && pale(x, y)) {
      seen[std::size_t{y} * w + x] = true;
      queue.emplace_back(x, y);
    }
  };
  for (std::uint32_t x = 0; x < w; ++x) visit(x, 0), visit(x, h - 1);
  for (std::uint32_t y = 0; y < h; ++y) visit(0, y), visit(w - 1, y);
  while (!queue.empty()) {
    const auto [x, y] = queue.front();
    queue.pop_front();
    picture->at(x, y) &= 0x00FFFFFF;
    if (x > 0) visit(x - 1, y);
    if (x + 1 < w) visit(x + 1, y);
    if (y > 0) visit(x, y - 1);
    if (y + 1 < h) visit(x, y + 1);
  }

  // Specks the background's pattern leaves (patches far smaller than the
  // logo) go too.
  constexpr std::size_t kSpeck = 64;
  std::vector<bool> done(std::size_t{w} * h);
  std::vector<std::pair<std::uint32_t, std::uint32_t>> patch;
  for (std::uint32_t sy = 0; sy < h; ++sy)
    for (std::uint32_t sx = 0; sx < w; ++sx) {
      if (done[std::size_t{sy} * w + sx] || !alpha(picture->at(sx, sy))) continue;
      patch.clear();
      patch.emplace_back(sx, sy);
      done[std::size_t{sy} * w + sx] = true;
      for (std::size_t i = 0; i < patch.size(); ++i) {
        const auto [x, y] = patch[i];
        auto add = [&](std::uint32_t nx, std::uint32_t ny) {
          if (!done[std::size_t{ny} * w + nx] && alpha(picture->at(nx, ny))) {
            done[std::size_t{ny} * w + nx] = true;
            patch.emplace_back(nx, ny);
          }
        };
        if (x > 0) add(x - 1, y);
        if (x + 1 < w) add(x + 1, y);
        if (y > 0) add(x, y - 1);
        if (y + 1 < h) add(x, y + 1);
      }
      if (patch.size() < kSpeck)
        for (const auto& [x, y] : patch) picture->at(x, y) &= 0x00FFFFFF;
    }
}

}  // namespace launcher
