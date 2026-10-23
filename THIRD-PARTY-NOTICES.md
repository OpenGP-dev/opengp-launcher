# Third-party notices

The OpenGP Launcher is Copyright (C) 2026 the OpenGP contributors, and is licensed under the GNU
General Public License, version 3 or (at your option) any later version (see [LICENSE](LICENSE)).
Its source code is this repository, <https://github.com/OpenGP-dev/opengp-launcher>, together with
OpenGP (<https://github.com/OpenGP-dev/OpenGP>): the launcher builds OpenGP's libraries as part of
itself. Built from this repository alone, it uses the OpenGP that its `external/OpenGP` submodule
points to; GitHub's source archives leave the submodule out, so clone with `--recursive`. The
launcher in an OpenGP installer package is built from the OpenGP release the package comes from, so
there the OpenGP part of its source is that release. `OpenGP --version` names both versions.

A launcher binary contains the third-party work below. Each keeps its own licence. A copy of the
launcher that is handed out must come with these notices and the licence texts they point to.

OpenGP's installer packages do. On Linux they also carry libraries that Qt needs: the xcb-util
libraries and ICU (the last two rows). In a package, this file is
`licenses/launcher-THIRD-PARTY-NOTICES.md`, the GPL is the package's `LICENSE`, and the other texts
are in `licenses/`: Qt's LGPL as `Qt-LGPL-3.0.txt`, the typeface's licence as
`MPLUSRounded1c-OFL.txt`, SDL's and toml++'s, and on Linux the xcb-util libraries' notices as
`xcb-util-COPYING.txt` and ICU's licence as `ICU-LICENSE.txt`.

| Work | Licence | Where |
|------|---------|-------|
| [Qt 6](https://www.qt.io/), 6.8 or later | LGPL-3.0 | the interface, linked as shared libraries |
| [SDL 3](https://github.com/libsdl-org/SDL), with its HIDAPI, yuv2rgb and libm code, and on Linux its X11 and Wayland code | zlib; BSD-3-Clause; BSD-3-Clause; Sun's free licence; MIT/X11 and HPND (Linux only) | the controllers, through OpenGP's controls code |
| [toml++](https://github.com/marzer/tomlplusplus) | MIT | reading and writing the games' files and the launcher's own |
| OpenGP's texture decoders, ported from [Dolphin](https://github.com/dolphin-emu/dolphin) | GPL-2.0-or-later, used under GPL-3.0 | the pictures from the player's disc image |
| M PLUS Rounded 1c | OFL-1.1 | `src/fonts/`, the interface's typeface, built in |
| SDL's scancode tables | zlib | `src/core/scancode_tables.cpp`, the keys pressed in the window |
| [xcb-util](https://xcb.freedesktop.org/): libxcb-cursor, -icccm, -image, -keysyms, -render-util and -util | MIT/X11; HPND (part of render-util) | Linux packages only: `qt/lib/`, for Qt's X11 support |
| [ICU](https://icu.unicode.org/), when Qt brings it | Unicode licence, and the notices in ICU's `LICENSE` | Linux packages only: `qt/lib/`, for Qt Core |

## Qt

The launcher's interface is built with Qt 6 (Qt Core, Qt Gui, Qt Network, Qt Qml, Qt Quick and Qt
Quick Controls, with the libraries and plugins they need), Copyright (C) The Qt Company Ltd. and
other contributors. The launcher uses Qt under the GNU Lesser General Public License, version 3
(LGPL-3.0). That is the GNU GPL version 3 ([LICENSE](LICENSE)) with the extra permissions in
[licenses/Qt-LGPL-3.0.txt](licenses/Qt-LGPL-3.0.txt), as Qt's source has them in
`LICENSES/LGPL-3.0-only.txt` (also at <https://www.gnu.org/licenses/lgpl-3.0.html>).

The launcher builds against Qt 6.8 or later, with no changes to Qt, and links it as shared
libraries, so they can be replaced with another build of the same Qt version. The source code of
every Qt release is at <https://download.qt.io/archive/qt/>, and in Qt's Git repositories at
<https://code.qt.io/>.

Qt contains third-party code of its own, such as PCRE2, HarfBuzz, FreeType, libpng and zlib,
depending on the system and on how Qt was built. Their licences and notices are listed in "Licenses
Used in Qt" (<https://doc.qt.io/qt-6/licenses-used-in-qt.html>), and in the `qt_attribution.json`
files and `LICENSES/` folders of Qt's source. The libraries Qt links on Linux that an OpenGP package
carries with it are under "xcb-util" and "ICU" below.

## SDL 3

The launcher reads the controllers through SDL 3, Copyright (C) 1997-2026 Sam Lantinga, under the
zlib licence. SDL includes HIDAPI (Copyright (c) 2010, Alan Ott, Signal 11 Software) and yuv2rgb
(Copyright (c) 2016, Adrien Descamps), both under the BSD 3-Clause licence, and maths functions from
Sun's fdlibm (Copyright (C) 1993 by Sun Microsystems, Inc.). On Linux it also includes code for its
X11 and Wayland video drivers, under the MIT/X11 and HPND licences: the X11 keysym table, the EDID
parser, the XSETTINGS client and the code generated from Wayland's protocol files.

This is the same SDL as in OpenGP's games and installer. The full text of each of these licences
and notices is under "SDL 3" in OpenGP's `THIRD-PARTY-NOTICES.md`, which is
`external/OpenGP/THIRD-PARTY-NOTICES.md` in the launcher's source. An OpenGP installer package has
that file too, with SDL's, HIDAPI's and yuv2rgb's licence files in its `licenses/` folder.

## toml++

The launcher and OpenGP's libraries read and write TOML files (the games' settings and controls,
`launcher.toml`, the news) with toml++, Copyright (c) Mark Gillard, under the MIT licence. Its text
is under "toml++" in OpenGP's `THIRD-PARTY-NOTICES.md`, and in
`external/OpenGP/thirdparty/tomlplusplus/LICENSE` in the launcher's source.

## Dolphin

The launcher makes its pictures (each game's logo, button and course previews) from the player's
disc image with OpenGP's texture decoders (`runtime/gx/texture.cpp` in OpenGP). They are ported from
Dolphin's `TextureDecoder_Generic.cpp`, `TextureDecoder_Util.h` and `LookUpTables.h`, Copyright
Dolphin Emulator Project, licensed under the GNU General Public License, version 2 or (at your
option) any later version. The launcher uses them under version 3, as that licence allows.

## M PLUS Rounded 1c

The interface's typeface, M PLUS Rounded 1c (Regular and ExtraBold), is Copyright 2016 The Rounded
M+ Project Authors, and is licensed under the SIL Open Font License, version 1.1
([src/fonts/OFL.txt](src/fonts/OFL.txt)). The launcher's copies are cut down to Latin letters and are
built into it.

## SDL's scancode tables

`src/core/scancode_tables.cpp` turns the keys pressed in the launcher's window into the SDL
scancodes the games name keys by. Its tables are SDL's (`src/events/scancodes_windows.h`,
`scancodes_linux.h` and `scancodes_darwin.h` of SDL 3.4), copied unchanged by
`tools/gen_scancodes.py`, under the zlib licence, whose text is in the file.

## xcb-util

On Linux, Qt draws the launcher's window through X11 with its xcb plugin, which needs the xcb-util
libraries. Not every desktop has them (libxcb-cursor above all), so an OpenGP package carries the
build machine's copies in `qt/lib/`: libxcb-cursor (from xcb-util-cursor), libxcb-icccm
(xcb-util-wm), libxcb-image (xcb-util-image), libxcb-keysyms (xcb-util-keysyms), libxcb-render-util
(xcb-util-renderutil) and libxcb-util (xcb-util), by Michael Stapelberg, Vincent Torri, Arnaud
Fontaine, Bart Massey, Julien Danjou, Keith Packard, Ian Osgood, Jamey Sharp, Josh Triplett and
Ulrich Eckhardt. They are under the MIT/X11 licence, and Keith Packard's part of xcb-util-renderutil
under an HPND licence. Each one's copyright and permission notice, as its `COPYING` file has it, is
in [licenses/xcb-util-COPYING.txt](licenses/xcb-util-COPYING.txt).

## ICU

Qt Core can use ICU (International Components for Unicode), Copyright © 1991-2023 Unicode, Inc.,
for text and locales. Qt's own Linux builds link ICU 73 and keep it in their `lib/` folder, so an
OpenGP package built with them carries it in `qt/lib/` (libicuuc, libicui18n and libicudata). ICU is
under the Unicode licence, and its `LICENSE` adds the notices of the third-party data and code it
contains, such as the word break dictionaries and Google's double-conversion. ICU 73's `LICENSE` is
in [licenses/ICU-LICENSE.txt](licenses/ICU-LICENSE.txt). A package that has no ICU in `qt/lib/`
doesn't carry the file either.
