# Changelog

## 1.0.1 (not released yet)

Launcher 1.0.1 goes with OpenGP 1.0.1: it is built on OpenGP 1.0.1's code, so it reads and writes
the games' files as the 1.0.1 games do. It still opens an OpenGP 1.0.0 package folder.

### Fixes

- Windows: the launcher now works in a folder, and with a disc image, whose path has characters
  other than A to Z, digits and simple punctuation (such as é, ü or 日本, as in a user folder
  named José): 1.0.0's known issue. In such a folder it used to show the default settings and
  controls, and its first change wrote them over the player's own, losing the held card, the
  linked cabinet settings and every custom binding. With an OpenGP 1.0.0 package the games
  themselves still don't work there: OpenGP 1.0.1 fixes them.
- One value OpenGP does not understand in `settings.toml` or `controls.toml` no longer makes the
  whole file unreadable on the Settings and Controls pages: the rest is read, as the 1.0.1 games
  read it. A `controls.toml` that can't be read at all is set aside as `controls.toml.bad` at
  once, as the games set it aside, and the Controls page can be used again.
- A game started from its own shortcut, or by an earlier launcher, now counts as running, as one
  started from the launcher does: its settings, controls and cards can only be looked at until it
  is closed. Changes made meanwhile used to be undone when the game saved. This works with OpenGP
  1.0.1's games, which hold `user/.lock-<ID>` while they run; 1.0.0's hold no such lock.
- **Stop** during an install now says the install is stopping. A compiler download or a compile
  that has already started finishes first.
- Closing the window during an install or a build stops it first, with the window still up. The
  launcher used to carry on with no window until the install had finished.

## 1.0.0 (2026-10-23)

The first release of the OpenGP Launcher, for OpenGP 1.0.0: one window to play Mario Kart Arcade GP
2 and GP 1, set them up and look after them. It builds OpenGP's own libraries from the
`external/OpenGP` submodule, so it reads and writes the games' files exactly as the games do. Like
OpenGP, the launcher is now feature-locked: from here on, releases bring only bug fixes and design
tweaks.

Known issue: on Windows, the launcher, like OpenGP 1.0.0, doesn't work when the path of OpenGP's
folder, or of a disc image you give it, has any character other than A to Z, digits and simple
punctuation (such as é, ü or 日本, as in a user folder named José). Put the package in a folder such
as `C:\Games\OpenGP`, and your images in its `ISO` folder (see OpenGP's
[docs/packaging.md](https://github.com/OpenGP-dev/OpenGP/blob/v1.0.0/docs/packaging.md#known-issue-on-windows)).
A patch release fixes this.

### The launcher

- **Home**: each game with the next thing to do (add its disc image, install or build it, update
  it, play it), and news from the OpenGP team, kept for when you are offline.
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
  their progress and can be stopped. A failed install keeps its output in `user/logs/`; a source
  build's output is in `build/<preset>/launcher-build.log`.
- **Settings**: video, sound, game, camera, linked cabinets and controls (keyboard, gamepads,
  wheels), written to the same `user/settings.toml` and `user/controls.toml` the games read. While a
  game runs they can only be looked at; they are read again when it ends.
- **Saves & data**: Mario Cards (hold, add, rename, copy, save as a file, delete), the game's own
  data (reset), backups of everything (back up, restore), screenshots and the last run's log.
- Before anything fails, a screen says what to do when OpenGP is opened from inside its zip, or
  from a package folder it cannot write to (such as Program Files). Opened where OpenGP isn't, the
  launcher says where to put it.

### Fixes

Since the launcher was first marked 1.0.0:

- macOS: a game started from the launcher no longer stops as it opens the webcam. macOS asks for
  the camera in the launcher's name, and the launcher now gives it the reason.
- Opened where OpenGP isn't (Downloads, Applications, or `OpenGP.app` moved out of its package),
  the launcher no longer writes anything there. It used to leave a `user` folder with
  `controls.toml`, `launcher.toml` and sometimes `settings.toml`.
- A `controls.toml` that OpenGP can't read (one input it does not know is enough) is no longer
  replaced by the default controls at the first change on the Controls page. The page says the file
  can't be read and only **Restore the defaults** works, which keeps the old file as
  `controls.toml.bad`.
- A launcher built from source says OpenGP 1.0.0, not 0.4.0, in `--version`, the play bar and
  Settings.
- macOS: the launcher opens on macOS 13 or later, as OpenGP does, whatever macOS it was built on.

### Licences

- `THIRD-PARTY-NOTICES.md` lists the third-party work in a launcher, with its licences and
  notices: Qt 6, SDL 3, toml++, OpenGP's texture decoders ported from Dolphin, the M PLUS Rounded 1c
  typeface and SDL's scancode tables. It also says where the source code is, Qt's included.
