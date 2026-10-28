OpenGP Launcher @VERSION@ for Linux (x86-64; glibc 2.35 or later, as Ubuntu 22.04)

The launcher for OpenGP, the native port of Mario Kart Arcade GP 2 and GP 1:
play either game, add your disc image, install the games, and change their
settings, controls and saves in one window.

Install
1. Unpack OpenGP (OpenGP-<version>-linux-x64.tar.gz) anywhere.
2. Copy everything in this archive's folder into that OpenGP folder, next to
   its sdk folder. Its OpenGP replaces the installer's: the launcher installs
   the games itself.
3. Run ./OpenGP. Add your disc image, install, play.

Qt needs a few of the system's libraries: on Debian or Ubuntu,
    sudo apt install libxcb-cursor0 libxkbcommon-x11-0 libegl1

Source: https://github.com/OpenGP-dev/opengp-launcher (GPL-3.0-or-later,
LICENSE.txt). Third-party notices: THIRD-PARTY-NOTICES.txt.
