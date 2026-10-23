# OpenGP Launcher

The launcher for [OpenGP](https://github.com/OpenGP-dev/OpenGP), the native PC port of Mario Kart
Arcade GP 2 and GP 1. In one window:

- **Home**: each game with the next thing to do (add its disc image, install or build it, play it),
  and news from the OpenGP team (see [docs/news.md](docs/news.md)).
- **Play** either game (GP 2 first; GP 1 when you have it), with one-off options: start in the test
  menu, or log the game's own messages.
- **Game management**: add your disc image of each game, and build the game from it: in an
  installed package, install it, update it after a new OpenGP, or uninstall it; in a source tree,
  build it with CMake.
- **Settings**: video, sound, game, camera, linked cabinets and controls (keyboard, gamepads,
  wheels), written to the same `user/settings.toml` and `user/controls.toml` the games read.
- **Saves & data**: Mario Cards (hold, add, rename, copy, save as a file, delete), the game's own
  data (reset), backups of everything (back up, restore), screenshots and the last run's log.

The pictures (each game's logo, button and course previews) come from your own disc image when you
add it, into `user/art/`; none are part of the launcher.

## Building

The launcher builds OpenGP's own libraries (settings, controls, disc images, the installer) as part
of itself, from the OpenGP submodule in `external/OpenGP`, so it always reads and writes the files
exactly as the games do.

You need what OpenGP needs (CMake 3.25+, Ninja, Clang or GCC; clang-cl on Windows) and **Qt 6.8 or
later** with Qt Quick, Qt Quick Controls and Qt Network. For example, with
[aqtinstall](https://github.com/miurahr/aqtinstall):

```sh
pip install aqtinstall
aqt install-qt windows desktop 6.10.3 win64_msvc2022_64 -O C:/Qt     # Linux: linux desktop 6.10.3 linux_gcc_64
```

```sh
git clone --recursive https://github.com/OpenGP-dev/opengp-launcher.git
cd opengp-launcher
cmake --preset default -DCMAKE_PREFIX_PATH=<Qt>/6.10.3/<kit>      # Windows: set QTDIR, then --preset clang-cl
cmake --build --preset default
ctest --preset default
```

Options:

| CMake variable | |
|---|---|
| `OPENGP_SOURCE_DIR` | another OpenGP checkout instead of the submodule (to work on both at once) |
| `LAUNCHER_DEV_HOME` | the OpenGP folder a development build looks after (default: `OPENGP_SOURCE_DIR`; empty for a release) |
| `LAUNCHER_NEWS_URL` | the news feed (`news.toml`); empty for none |

OpenGP's package workflow builds the launcher for each installer package, from the OpenGP release it
packages, and installs it into the package folder. `OpenGP` (`OpenGP.exe`, `OpenGP.app`) goes at the
top, with the Qt it needs, the Visual C++ runtime on Windows, and its licences in `licenses/`. Qt's
own deployment does the Qt part: windeployqt on Windows, macdeployqt on macOS (into `OpenGP.app`),
and on Linux the libraries, plugins and QML modules in `qt/`, with `qt.conf` next to the launcher.
On Linux the launcher uses X11 (XWayland on a Wayland desktop). The package carries the xcb-util
libraries that Qt needs and not every desktop has, so the build machine needs them:
`libxcb-cursor0`, `libxcb-icccm4`, `libxcb-image0`, `libxcb-keysyms1` and `libxcb-render-util0`. A
release build has no development folder to fall back on, so outside a package it looks only in its
own folder:

```sh
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<Qt>/<version>/<kit> \
  -DLAUNCHER_DEV_HOME= -DLAUNCHER_NEWS_URL= -DOPENGP_SOURCE_DIR=<the OpenGP release being packaged>
# Windows: in OpenGP's dev shell, add -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
cmake --build build/release
cmake --install build/release --prefix <the package folder>
```

## Running

In an installed OpenGP package the launcher is `OpenGP` (`OpenGP.exe`, `OpenGP.app`), the program
players run, and looks after the folder it is in: it builds each game there from the player's disc
image with OpenGP's installer code, which on Windows also downloads the compiler. The package's
console installer, for the command line, is `opengp-install` (`opengp-install.exe`) next to it. A
development build looks after `LAUNCHER_DEV_HOME`, which is an OpenGP source tree: it plays the
games CMake built there (`build/*/games/`), and its Build button builds them there with CMake
(`source_build`): it puts the game's `main.dol` from your disc image in `orig/`, configures the tree
(the `default` preset, `clang-cl` on Windows, or the build folder already there) and builds the
game, with CMake's output in `build/<preset>/launcher-build.log`. On Windows, start the launcher
from OpenGP's dev shell (`tools/devshell.ps1`) so CMake finds clang-cl.

```
OpenGP [--home DIR] [--user-dir DIR] [--game ID] [--play] [IMAGE...]
                                             --play: start the game at once (shortcuts)
                                             IMAGE: disc images to add (dropped on OpenGP)
OpenGP --version
OpenGP --smoke-test                          load the interface and quit (fails on any QML warning)
OpenGP --screenshot-ui FILE [--view VIEW]    a picture of the window, then quit
                                             VIEW: home, play, saves, settings/<page>
```

Settings and saves are in `<home>/user/` (or `--user-dir`): the games' `settings.toml`,
`controls.toml`, cards and data; the launcher's own `launcher.toml`; `art/`, `logs/` (each game's
last two runs), `backups/` and `news.toml` (the news last downloaded).

## How it works

| | |
|---|---|
| `src/core/` | plain C++ on OpenGP's libraries, tested in `tests/`: finding OpenGP (`home`), the games' states (`library`), disc images against the known good dumps (`dump_check`), running a game (`game_process`), building a source tree's games (`source_build`), pictures from TPL textures (`art`), cards and backups (`user_data`), the news feed (`news`), the launcher's settings (`prefs`), keys as SDL scancodes (`scancodes`) |
| `src/qt/` | what QML sees: `Launcher` (games, play, build, add image), `GameSettings`, `Controls`, `UserData`, `News`, and the pictures (`art_maker`) |
| `src/qml/` | the interface (Qt Quick) |

Things it takes care of:

- **The games rewrite `settings.toml` whole** when they save, from what they read at start-up, so
  while a game runs its settings can only be looked at here; they are read again when it ends. The
  launcher's own settings are in `launcher.toml`.
- **A game restarts itself** for its test menu: on Windows that is a new process, so the launcher runs
  each game in a job object and waits for every process in it (`game_process.cpp`).
- **Controls** are edited with OpenGP's own `input::HostInput`. Controllers come through SDL3 (no SDL
  window); keys pressed in the launcher's window become the SDL scancodes the games name keys by,
  through SDL's own tables (`scancode_tables.cpp`, made by `tools/gen_scancodes.py`). While a game
  runs, the launcher lets go of the controllers.

## Features

The launcher is feature-locked at 1.0, like OpenGP: from here on it takes only bug fixes and design
tweaks. Besides what is above:

| # | Feature |
|---|---------|
| L1 | Disc image checking: each image checked against the known good dump of its game (the SHA-1 of the image after its header, from FIX94's [triforce-header-patcher](https://github.com/FIX94/triforce-header-patcher)), in the background, on the Play tab (`dump_check`) (**done**) |
| L2 | Drag and drop: drop an `.iso` or `.gcm` on the window to add it, as **Add disc image** does (**done**) |

## Licence

Copyright (C) 2026 the OpenGP contributors. GPL-3.0-or-later, like OpenGP ([LICENSE](LICENSE)). The
source code is this repository and OpenGP at the commit `external/OpenGP` points to.

A launcher also contains third-party work, each under its own licence: Qt 6 (LGPL-3.0,
[licenses/Qt-LGPL-3.0.txt](licenses/Qt-LGPL-3.0.txt)), SDL 3 with HIDAPI and yuv2rgb, toml++,
OpenGP's texture decoders ported from Dolphin, the interface's typeface M PLUS Rounded 1c (SIL Open
Font Licence, [src/fonts/OFL.txt](src/fonts/OFL.txt)), cut down to Latin letters, and SDL's scancode
tables (zlib licence, in the file). [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) lists them with
their notices, and says where Qt's source is. An OpenGP package carries it as
`licenses/launcher-THIRD-PARTY-NOTICES.md`, with the licence texts it points to.
