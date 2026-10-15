"""Writes src/core/scancode_tables.cpp from SDL's scancode tables.

    python tools/gen_scancodes.py <SDL source>/src/events src/core/scancode_tables.cpp
"""
import re
import sys
from pathlib import Path

sdl = Path(sys.argv[1])  # SDL's src/events
out = Path(sys.argv[2])

header = (sdl / "scancodes_windows.h").read_text(encoding="utf-8")
license_text = header[header.index("/*") + 2:header.index("*/")]
license_lines = ["//" + (" " + l.strip() if l.strip() else "") for l in license_text.strip("\n").splitlines()]


def table(file: str, name: str) -> list[str]:
    text = (sdl / file).read_text(encoding="utf-8")
    start = text.index(name)
    body = text[text.index("{", start) + 1:text.index("};", start)]
    body = re.sub(r"#if 0.*?#endif[^\n]*", "", body, flags=re.S)  # entries SDL leaves out
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    return [t.strip() for t in body.split(",") if t.strip()]


def emit(name: str, entries: list[str]) -> str:
    # extern: a const array would otherwise be private to this file.
    lines = [f"extern const SDL_Scancode {name}[];", f"extern const std::size_t {name}_size;",
             f"const SDL_Scancode {name}[] = {{"]
    for i in range(0, len(entries), 4):
        lines.append("    " + ", ".join(entries[i:i + 4]) + ",")
    lines.append("};")
    lines.append(f"const std::size_t {name}_size = {len(entries)};")
    return "\n".join(lines)


parts = [
    "// SPDX-License-Identifier: Zlib",
    "//",
    "// Native key codes to SDL scancodes: SDL's tables (src/events/scancodes_windows.h,",
    "// scancodes_linux.h and scancodes_darwin.h of SDL 3.4), copied unchanged by",
    "// tools/gen_scancodes.py. SDL's licence:",
    "//",
    *license_lines,
    "",
    "#include <cstddef>",
    "",
    "#include <SDL3/SDL_scancode.h>",
    "",
    "namespace launcher::scancode_tables {",
    "",
    "// Indexed by the PC scan code, with 0x80 added for extended (0xE0) keys.",
    emit("windows_table", table("scancodes_windows.h", "windows_scancode_table")),
    "",
    "// Indexed by the Linux input event code (X11 and Wayland key codes are 8 more).",
    emit("linux_table", table("scancodes_linux.h", "linux_scancode_table")),
    "",
    "// Indexed by the macOS virtual key code (kVK_...).",
    emit("darwin_table", table("scancodes_darwin.h", "darwin_scancode_table")),
    "",
    "}  // namespace launcher::scancode_tables",
    "",
]
out.write_text("\n".join(parts), encoding="utf-8", newline="\n")
print(out, "windows/linux/darwin:", *(len(table(f, n)) for f, n in [
    ("scancodes_windows.h", "windows_scancode_table"),
    ("scancodes_linux.h", "linux_scancode_table"),
    ("scancodes_darwin.h", "darwin_scancode_table")]))
