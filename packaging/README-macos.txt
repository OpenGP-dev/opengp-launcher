OpenGP Launcher @VERSION@ for macOS (Apple Silicon, macOS 13 or later)

The launcher for OpenGP, the native port of Mario Kart Arcade GP 2 and GP 1:
play either game, add your disc image, install the games, and change their
settings, controls and saves in one window.

Install
1. Unpack OpenGP (OpenGP-<version>-macos-arm64.tar.gz) anywhere.
2. Put OpenGP.app from this archive in that OpenGP folder, next to its sdk
   folder.
3. Open OpenGP.app. Add your disc image, install, play.

The app isn't signed by Apple. The first time, right-click OpenGP.app and
choose Open, or run in Terminal:
    xattr -dr com.apple.quarantine <the OpenGP folder>

If your disc image is in Downloads, Documents or Desktop, macOS asks once
whether OpenGP may use that folder.

Source: https://github.com/OpenGP-dev/opengp-launcher (GPL-3.0-or-later,
LICENSE.txt). Third-party notices: THIRD-PARTY-NOTICES.txt.
