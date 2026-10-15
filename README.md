# OpenGP Launcher

The launcher for [OpenGP](https://github.com/OfficiallySp/OpenGP), the native PC port of Mario Kart
Arcade GP 2 and GP 1. In one window:

- **Play** either game (GP 2 first; GP 1 when you have it), with one-off options: start in the test
  menu, free play, the game's own log.
- **Game management**: add your disc image of each game, and, in an installed package, build
  (install) the game from it, update it after a new OpenGP, or uninstall it.
- **Settings**: video, sound, game, camera, linked cabinets and controls (keyboard, gamepads,
  wheels), written to the same `user/settings.toml` and `user/controls.toml` the games read.
- **Saves & data**: Mario Cards (hold, add, rename, copy, save as a file, delete), the game's own
  data (reset), backups of everything (back up, restore), screenshots and the last run's log.
- **News** from the OpenGP team (see [docs/news.md](docs/news.md)).

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
| `LAUNCHER_DEV_HOME` | the OpenGP folder a development build looks after (default: `OPENGP_SOURCE_DIR`) |
| `LAUNCHER_NEWS_URL` | the news feed (`news.toml`); empty for none |

## Running

In an installed OpenGP package the launcher is `OpenGP` (`OpenGP.exe`, `OpenGP.app`) and looks after
the folder it is in. A development build looks after `LAUNCHER_DEV_HOME`, which is an OpenGP source
tree: it plays the games CMake built there (`build/*/games/`), and cannot install them itself.

```
OpenGP [--home DIR] [--user-dir DIR] [--game ID] [--play]    --play: start the game at once (shortcuts)
OpenGP --version
OpenGP --smoke-test                          load the interface and quit (fails on any QML warning)
OpenGP --screenshot-ui FILE [--view VIEW]    a picture of the window, then quit
                                             VIEW: play, saves, settings/<page>
```

Settings and saves are in `<home>/user/` (or `--user-dir`): the games' `settings.toml`,
`controls.toml`, cards and data; the launcher's own `launcher.toml`; `art/`, `logs/` (each game's
last two runs), `backups/` and `news.toml` (the news last downloaded).

## How it works

| | |
|---|---|
| `src/core/` | plain C++ on OpenGP's libraries, tested in `tests/`: finding OpenGP (`home`), the games' states (`library`), running a game (`game_process`), pictures from TPL textures (`art`), cards and backups (`user_data`), the news feed (`news`), the launcher's settings (`prefs`), keys as SDL scancodes (`scancodes`) |
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

## Licence

GPL-3.0-or-later, like OpenGP ([LICENSE](LICENSE)). The interface's typeface is M PLUS Rounded 1c
(SIL Open Font Licence, [src/fonts/OFL.txt](src/fonts/OFL.txt)), cut down to Latin letters. The
scancode tables are SDL's (zlib licence, in the file).
