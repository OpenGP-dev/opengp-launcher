// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

// When an update is out: a notice at the top of every page that installs it
// (or, when it can't, links to its page), and says how installing it is
// going. The cross hides it until the launcher is opened again.
Rectangle {
    id: root
    property string dismissed  // the update whose notice was closed
    readonly property bool busy: Updater.state !== Updater.Idle

    visible: Updater.name.length > 0 && (busy || Updater.name !== dismissed)
    implicitWidth: notice.implicitWidth + 40
    implicitHeight: notice.implicitHeight + 14
    radius: height / 2
    color: Qt.rgba(0.082, 0.102, 0.145, 0.92)
    border.color: Theme.line

    function text() {
        switch (Updater.state) {
        case Updater.Downloading: return "Downloading " + Updater.name + ": " + Math.floor(Updater.progress * 100) + "%"
        case Updater.Unpacking: return "Unpacking " + Updater.name
        case Updater.Waiting:
            return Updater.name + (Launcher.buildingId.length > 0 ? " installs when the build ends."
                                                                  : " installs when the game closes.")
        case Updater.Installing: return "Installing " + Updater.name
        default: return Updater.name + " is out."
        }
    }

    RowLayout {
        id: notice
        anchors.centerIn: parent
        spacing: 12
        Rectangle {
            width: 8; height: 8; radius: 4
            color: Theme.blue
        }
        Text {
            text: root.text()
            color: Theme.chalk
            font.pixelSize: Theme.lead
        }
        PillButton {
            visible: !root.busy && Updater.canInstall
            kind: "primary"
            text: "Update"
            onClicked: confirm.ask("Update to " + Updater.name + "?",
                                   "OpenGP downloads it (" + Updater.sizeMB + " MB), puts it in place of this version "
                                   + "and opens again. Your saves, settings and disc images stay as they are."
                                   + (Updater.rebuilds ? " Then choose Update on each game you installed, to build it "
                                                         + "for the new version." : ""),
                                   "Update", false, () => Updater.install())
        }
        PillButton {
            visible: !root.busy
            text: Updater.canInstall ? "What's new" : "Download"
            onClicked: Launcher.openLink(Updater.page)
        }
        PillButton {
            visible: Updater.state === Updater.Downloading || Updater.state === Updater.Unpacking
                     || Updater.state === Updater.Waiting
            text: "Stop"
            onClicked: Updater.cancel()
        }
        Icon {
            visible: !root.busy
            implicitWidth: 12
            implicitHeight: 12
            name: "cross"
            color: closeArea.containsMouse ? Theme.chalk : Theme.faint
            MouseArea {
                id: closeArea
                anchors.fill: parent
                anchors.margins: -6
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.dismissed = Updater.name
            }
        }
    }

    ConfirmDialog { id: confirm }
}
