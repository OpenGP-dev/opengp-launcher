// SPDX-License-Identifier: GPL-3.0-or-later
//
// Keys pressed in the launcher's (Qt) window, as the SDL scancodes the games
// bind keys by: user/controls.toml names keys with SDL_GetScancodeName, so a
// key bound here must be the one the game sees.
#pragma once

#include <cstdint>

namespace launcher {

// The SDL scancode (an SDL_Scancode) of a key event, from its native codes:
// Qt's QKeyEvent::nativeScanCode() (Windows, Linux) and nativeVirtualKey()
// (macOS, where Qt has no scan code). 0 (SDL_SCANCODE_UNKNOWN) if unknown.
int sdl_scancode(std::uint32_t native_scan_code, std::uint32_t native_virtual_key);

}  // namespace launcher
