// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic

// The one thing to do next with a game: add its disc image, install it (in a
// source tree, build it), update it or play it. The play bar's big button,
// and each game's on Home.
Button {
    id: button
    property int index: -1     // in Launcher.games
    property string gameId
    property int gameState: Launcher.Checking
    property bool joined: false  // the run options' button is beside it, on the right
    signal addImage()

    readonly property bool building: Launcher.buildingId.length > 0 && Launcher.buildingId === gameId
    readonly property bool buildingOther: Launcher.buildingId.length > 0 && !building
    readonly property bool running: Launcher.runningId.length > 0
    readonly property bool runningThis: running && Launcher.runningId === gameId

    hoverEnabled: true
    font.family: Theme.family
    font.pixelSize: 24
    font.weight: Font.ExtraBold
    enabled: {
        if (building || running || buildingOther) return gameState === Launcher.NoImage && !building
        return gameState !== Launcher.Checking
    }
    text: {
        if (building) return "Building"
        if (runningThis) return "Playing"
        switch (gameState) {
        case Launcher.Checking: return "Checking"
        case Launcher.NoImage: return "Add disc image"
        case Launcher.NotBuilt: return Launcher.isPackage ? "Install" : "Build"
        case Launcher.OutOfDate: return "Update"
        default: return "Play"
        }
    }
    onClicked: {
        switch (gameState) {
        case Launcher.NoImage: button.addImage(); break
        case Launcher.NotBuilt:
        case Launcher.OutOfDate: Launcher.build(index); break
        case Launcher.Ready: Launcher.play(index); break
        }
    }

    contentItem: Text {
        text: button.text
        font: button.font
        color: button.enabled ? Theme.inkOnMarquee : Theme.dim
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 12
        topRightRadius: button.joined ? 4 : 12
        bottomRightRadius: button.joined ? 4 : 12
        color: !button.enabled ? Theme.panel
               : button.down ? Theme.marqueeDark : button.hovered ? "#FFD24F" : Theme.marquee
        border.width: button.visualFocus ? 2 : 0
        border.color: Theme.chalk
    }
}
