// SPDX-License-Identifier: GPL-3.0-or-later
//
// Pictures for the launcher from the player's own disc image: the games'
// textures (TPL files), decoded with OpenGP's GX texture decoder. They are
// made on the player's machine into user/art/<ID>/ and never shipped.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace launcher {

struct Picture {
  std::uint32_t width = 0, height = 0;
  std::vector<std::uint32_t> pixels;  // RGBA8, red in the lowest byte, rows top to bottom

  std::uint32_t& at(std::uint32_t x, std::uint32_t y) { return pixels[std::size_t{y} * width + x]; }
  std::uint32_t at(std::uint32_t x, std::uint32_t y) const { return pixels[std::size_t{y} * width + x]; }
};

// The games wrap some files in zlib: a little-endian u32 size, four zero
// bytes, then the zlib stream. Returns the stream's expected size, or 0 if
// `data` is not wrapped.
std::uint32_t wrapped_size(const std::vector<std::uint8_t>& data);

// The images in a TPL file (the GameCube's texture container), base levels
// only.
bool decode_tpl(const std::vector<std::uint8_t>& tpl, std::vector<Picture>* pictures, std::string* error);

// Full-screen pictures (course previews, title screens) are 640 pixels wide,
// stored as power-of-two tiles side by side (512 + 128, or 256 + 256 + 128),
// and sometimes as two such rows. Joins the tiles into whole pictures.
std::vector<Picture> stitch_screens(const std::vector<Picture>& tiles);

// GP 2 keeps some textures' transparency in a separate grey texture
// (<name>_a.tpl): multiplies the picture's alpha by the mask's grey.
bool apply_mask(Picture* picture, const Picture& mask);

// The part of a picture inside a rectangle (clipped to it).
Picture crop(const Picture& picture, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height);

// The picture cut down to its pixels that are not fully transparent.
Picture trim(const Picture& picture);

// Makes the pale, unsaturated background around a picture transparent: every
// such pixel connected to the picture's edge (a logo on a white title screen),
// and the specks of its pattern left over.
void key_out_background(Picture* picture);

}  // namespace launcher
