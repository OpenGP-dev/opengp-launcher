# Changelog

## 1.0.0 (2026-11-02)

The first release of the OpenGP Launcher, with OpenGP 1.0.1, OpenGP's first public release. The
launcher is one window to play Mario Kart Arcade GP 2 and GP 1, set them up and look after them. It
comes in OpenGP 1.0.1's installer packages as `OpenGP` (`OpenGP.exe`, `OpenGP.app`), the program
players run, with the Qt it needs. It builds OpenGP 1.0.1's own libraries from the `external/OpenGP`
submodule, so it reads and writes the games' files exactly as the games do. It also opens an
OpenGP 1.0.0 package folder.

### The launcher

- **Home**: each game with the next thing to do (add its disc image, install or build it, update
  it, play it), and news from the OpenGP team, kept for when you are offline.
- **Update notices**: in an installed package, Home says when a newer OpenGP is out ("OpenGP 1.0.2
  is out."), with a **Download** button that opens its release page on GitHub. The launcher looks
  at OpenGP's latest release when it starts and every half hour, and not at all with the news
  turned off (Settings, Launcher). It downloads nothing else and changes nothing.
- **Play** either game (GP 2 first; GP 1 when you have it), with one-off options: start in the test
  menu, or log the game's own messages. A game runs without a console window, with its output in
  `user/logs/`. `--play` starts the game at once, for shortcuts. The launcher follows a game that
  restarts itself for its test menu until it really ends.
- When a game ends, the launcher says why it could not run, that it crashed (naming
  `user/crash-report.txt`) or that it stopped responding (naming `user/hang-report.txt`), and shows
  the save failures the game could not show.
- **Disc images**: add one with **Add disc image**, by dropping an `.iso` or `.gcm` on the window,
  or by dropping it on the launcher itself. Each image is checked in the background against the
  known good dump of its game, and the Play tab shows the result. When a file can't be used, the
  launcher says why: a compressed image (and how to convert it), not a disc, or another game or
  region.
- **Building the games**: in an installed package, install them with OpenGP's installer, update
  them after a new OpenGP, or uninstall them; in a source tree, build them with CMake. Both show
  their progress and can be stopped: **Stop** says the install is stopping, and a compiler download
  or a compile that has already started finishes first. Closing the window during an install or a
  build stops it first, with the window still up. A failed install keeps its output in
  `user/logs/`; a source build's output is in `build/<preset>/launcher-build.log`.
- **Settings**: video, sound, game, camera, linked cabinets and controls (keyboard, gamepads,
  wheels), written to the same `user/settings.toml` and `user/controls.toml` the games read. While a
  game runs, whether the launcher started it or not (its own shortcut, another launcher), they can
  only be looked at; they are read again when it ends. One value OpenGP does not understand in
  either file leaves the rest readable, as the games read it. A `controls.toml` that can't be read
  at all is set aside as `controls.toml.bad`, as the games set it aside, and the default controls
  are used; if it can't be set aside, the Controls page changes nothing until you restore the
  defaults.
- **Saves & data**: Mario Cards (hold, add, rename, copy, save as a file, delete), the game's own
  data (reset), backups of everything (back up, restore), screenshots and the last run's log.
- Before anything fails, a screen says what to do when OpenGP is opened from inside its zip, or
  from a package folder it cannot write to (such as Program Files). Opened where OpenGP isn't
  (Downloads, Applications, or `OpenGP.app` moved out of its package), the launcher says where to
  put it, and writes nothing there.
- Paths with any characters: on Windows the launcher works in a folder, and with a disc image,
  whose path has characters such as é, ü or 日本 (as in a user folder named José).
- macOS: the launcher runs on macOS 13 or later, as OpenGP does, and gives macOS its reason for
  asking for the camera, so a game it starts can use the webcam.

### Licences

- `THIRD-PARTY-NOTICES.md` lists the third-party work in a launcher, with its licences and
  notices: Qt 6, SDL 3, toml++, OpenGP's texture decoders ported from Dolphin, the M PLUS Rounded 1c
  typeface and SDL's scancode tables. It also says where the source code is, Qt's included.
- An OpenGP package carries these notices as `licenses/launcher-THIRD-PARTY-NOTICES.md`, with the
  texts they point to: Qt's LGPL (`licenses/Qt-LGPL-3.0.txt`) and the typeface's licence
  (`licenses/MPLUSRounded1c-OFL.txt`). On Linux it also has the notices of the libraries it carries
  for Qt: the xcb-util libraries' (`licenses/xcb-util-COPYING.txt`) and ICU's
  (`licenses/ICU-LICENSE.txt`).
