// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton
import QtQuick

// The launcher's look: night asphalt, with the cabinets' marquee yellow for
// the one thing to press, GP 2's blue for links and focus, signal red for
// trouble and green for a check passed. One rounded typeface (M PLUS Rounded 1c), two weights.
QtObject {
    readonly property color asphalt: "#1D2331"      // the window
    readonly property color asphaltDeep: "#151A25"  // the game rail, the play bar
    readonly property color panel: "#272F41"        // raised surfaces
    readonly property color panelHover: "#313A4F"
    readonly property color line: "#3A4459"         // hairlines between things
    readonly property color marquee: "#FFC629"      // Play, the chosen game
    readonly property color marqueeDark: "#E0A800"
    readonly property color inkOnMarquee: "#1B1405"
    readonly property color blue: "#3B9BEF"         // links, focus, switches
    readonly property color red: "#E5243B"          // errors, a game running
    readonly property color green: "#3FBF74"        // a check passed (a good disc image)
    readonly property color chalk: "#F2F4F8"        // text
    readonly property color dim: "#98A3B6"          // secondary text
    readonly property color faint: "#6B768A"

    readonly property string family: "Rounded Mplus 1c"  // M PLUS Rounded 1c's family name in the font files

    // Type scale (pixels).
    readonly property int small: 12
    readonly property int body: 14
    readonly property int lead: 16
    readonly property int title: 20
    readonly property int heading: 26
    readonly property int display: 34

    readonly property int radius: 10
    readonly property int gap: 16
}
