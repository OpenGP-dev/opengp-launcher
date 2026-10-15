// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/scancodes.h"

#include <cstddef>

#include <SDL3/SDL_scancode.h>

namespace launcher {

namespace scancode_tables {
extern const SDL_Scancode windows_table[];
extern const std::size_t windows_table_size;
extern const SDL_Scancode linux_table[];
extern const std::size_t linux_table_size;
extern const SDL_Scancode darwin_table[];
extern const std::size_t darwin_table_size;
}  // namespace scancode_tables

int sdl_scancode(std::uint32_t native_scan_code, std::uint32_t native_virtual_key) {
  using namespace scancode_tables;
  auto lookup = [](const SDL_Scancode* table, std::size_t size, std::uint32_t index) {
    return index < size ? static_cast<int>(table[index]) : static_cast<int>(SDL_SCANCODE_UNKNOWN);
  };
#if defined(_WIN32)
  // Qt passes the scan code from WM_KEYDOWN with 0x100 set for extended
  // (0xE0) keys. As SDL does (WindowsScanCodeToSDLScanCode): the extended bit
  // becomes 0x80 of the index, and Pause (0x45, not extended) is 0xE046.
  (void)native_virtual_key;
  std::uint32_t code = native_scan_code & 0x7F;
  bool extended = (native_scan_code & 0x100) != 0;
  if (code == 0) return SDL_SCANCODE_UNKNOWN;
  if (code == 0x45 && !extended) code = 0x46, extended = true;
  return lookup(windows_table, windows_table_size, code | (extended ? 0x80 : 0));
#elif defined(__APPLE__)
  (void)native_scan_code;
  return lookup(darwin_table, darwin_table_size, native_virtual_key);
#else
  // X11 and Wayland key codes are the kernel's input event codes plus 8.
  (void)native_virtual_key;
  if (native_scan_code < 8) return SDL_SCANCODE_UNKNOWN;
  return lookup(linux_table, linux_table_size, native_scan_code - 8);
#endif
}

}  // namespace launcher
