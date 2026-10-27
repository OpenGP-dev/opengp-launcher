import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The chosen game's disc image and build (the news is on Home).
Item {
    id: tab
    property var game: ({})
    signal addImage()

    // Scrolls when the window is too short for all of it.
    ScrollView {
        anchors.fill: parent
        anchors.leftMargin: 48
        anchors.rightMargin: 24
        anchors.bottomMargin: 24
        contentWidth: availableWidth
        clip: true

        // About this game's files.
        Rectangle {
            width: 380
            implicitHeight: info.implicitHeight + 40
            radius: 14
            color: Qt.rgba(0.082, 0.102, 0.145, 0.78)
            border.color: Theme.line

            ColumnLayout {
                id: info
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                Text {
                    text: "Disc image"
                    color: Theme.chalk
                    font.pixelSize: Theme.lead
                    font.weight: Font.ExtraBold
                }
                Text {
                    Layout.fillWidth: true
                    text: tab.game.image ? tab.game.image.split(/[\\/]/).pop() : "Not Found"
                    color: tab.game.image ? Theme.chalk : Theme.dim
                    font.pixelSize: Theme.body
                    elide: Text.ElideMiddle
                }
                Text {
                    Layout.fillWidth: true
                    visible: !!tab.game.image
                    text: tab.game.image || ""
                    color: Theme.faint
                    font.pixelSize: Theme.small
                    elide: Text.ElideMiddle
                }
                // Against the known good dumps, checked in the background.
                RowLayout {
                    Layout.fillWidth: true
                    visible: !!tab.game.image
                    spacing: 8
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 6
                        width: 8; height: 8; radius: 4
                        color: tab.dumpColor()
                    }
                    Text {
                        Layout.fillWidth: true
                        text: tab.dumpText()
                        color: Theme.chalk
                        font.pixelSize: Theme.body
                        wrapMode: Text.WordWrap
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: !!tab.game.image && text.length > 0 && !tab.verifying
                    text: tab.game.dumpCheck === Launcher.DumpGood ? (tab.game.dumpName || "") : (tab.game.dumpNote || "")
                    color: Theme.dim
                    font.pixelSize: Theme.small
                    wrapMode: Text.WordWrap
                    lineHeight: 1.2
                }
                // The last disc image chosen for this game that could not be
                // added, and why: it stays until dismissed or an image is added.
                Rectangle {
                    Layout.fillWidth: true
                    visible: !!tab.game.imageError
                    implicitHeight: imageErrorRow.implicitHeight + 24
                    radius: 10
                    color: Qt.rgba(0.898, 0.141, 0.231, 0.14)
                    border.color: Qt.rgba(0.898, 0.141, 0.231, 0.5)
                    RowLayout {
                        id: imageErrorRow
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text {
                                text: "Invalid Disk Image"
                                color: Theme.chalk
                                font.pixelSize: Theme.body
                                font.weight: Font.ExtraBold
                            }
                            TextEdit {
                                Layout.fillWidth: true
                                text: tab.game.imageError || ""
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap
                                color: Theme.chalk
                                selectionColor: Theme.blue
                                font.family: Theme.family
                                font.pixelSize: Theme.small
                            }
                        }
                        Text {
                            Layout.alignment: Qt.AlignTop
                            text: "\u2715"
                            color: Theme.dim
                            font.pixelSize: Theme.body
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -8
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Launcher.dismissImageError(Launcher.currentIndex)
                            }
                        }
                    }
                }
                // No image: where the old one was, and why files in the ISO
                // folder were passed over (a compressed image, another region...).
                Text {
                    Layout.fillWidth: true
                    visible: !tab.game.image && text.length > 0
                    text: {
                        let lines = []
                        if (tab.game.imageNote) lines.push(tab.game.imageNote)
                        for (const line of Launcher.skippedImages) lines.push(line)
                        if (lines.length === 0) return ""
                        return lines.join("\n")
                    }
                    color: Theme.dim
                    font.pixelSize: Theme.small
                    wrapMode: Text.WordWrap
                    lineHeight: 1.2
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    PillButton {
                        text: tab.game.image ? "Change" : "Add disc image"
                        enabled: Launcher.runningId.length === 0 && Launcher.buildingId.length === 0
                        onClicked: tab.addImage()
                    }
                    PillButton {
                        text: "Open ISO folder"
                        visible: !tab.game.image
                        onClicked: Launcher.openFolder("iso")
                    }
                    PillButton {
                        text: "Show file"
                        visible: !!tab.game.image
                        onClicked: Launcher.openFolder("image")
                    }
                    PillButton {
                        text: "Verify"
                        visible: !!tab.game.image && tab.game.dumpCheck !== Launcher.NotChecked
                        enabled: Launcher.verifyingId.length === 0
                        onClicked: Launcher.checkImage(Launcher.currentIndex)
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line; Layout.topMargin: 6 }

                Text {
                    text: "Game"
                    color: Theme.chalk
                    font.pixelSize: Theme.lead
                    font.weight: Font.ExtraBold
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.dim
                    font.pixelSize: Theme.body
                    lineHeight: 1.2
                    text: {
                        switch (tab.game.gameState) {
                        case Launcher.Ready: return Launcher.isPackage ? "Installed, built by the latest version of OpenGP." : "Built from source"
                        case Launcher.OutOfDate: return "Built by an old or outdated version of OpenGP."
                        case Launcher.NotBuilt: return Launcher.isPackage ? "Not installed yet." : "Not built yet."
                        case Launcher.NoImage: return "No Disk Image."
                        default: return "Checking"
                        }
                    }
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    PillButton {
                        text: "Rebuild"
                        visible: tab.game.gameState === Launcher.Ready
                        enabled: Launcher.runningId.length === 0 && Launcher.buildingId.length === 0
                        onClicked: confirm.ask("Build " + tab.game.title + " again?",
                                               Launcher.isPackage ? "OpenGP builds the game from your disc image again. It takes a few minutes."
                                                                  : "CMake builds the game from this source tree again, with your disc image.",
                                               "Build again", false, () => Launcher.build(Launcher.currentIndex))
                    }
                    PillButton {
                        text: "Uninstall"
                        visible: Launcher.isPackage && (tab.game.gameState === Launcher.Ready || tab.game.gameState === Launcher.OutOfDate)
                        enabled: Launcher.runningId.length === 0 && Launcher.buildingId.length === 0
                        onClicked: confirm.ask("Uninstall " + tab.game.title + "?",
                                               "This removes the built game. Your disc image, settings, cards and records stay.",
                                               "Uninstall", true, () => Launcher.removeBuild(Launcher.currentIndex))
                    }
                    PillButton {
                        text: "Open OpenGP folder"
                        onClicked: Launcher.openFolder("home")
                    }
                }
            }
        }
    }

    ConfirmDialog { id: confirm }

    readonly property bool verifying: Launcher.verifyingId.length > 0 && Launcher.verifyingId === game.gameId
    function dumpText() {
        if (verifying) return "Checking it against the known good dump: " + Math.round(Launcher.verifyProgress * 100) + "%"
        switch (game.dumpCheck) {
        case Launcher.DumpGood: return "Known good dump"
        case Launcher.DumpKnownBad: return "Known bad dump"
        case Launcher.DumpUnknown: return "Unknown dump"
        default: return "Not checked yet"
        }
    }
    function dumpColor() {
        if (verifying) return Theme.blue
        switch (game.dumpCheck) {
        case Launcher.DumpGood: return Theme.green
        case Launcher.DumpKnownBad: return Theme.red
        case Launcher.DumpUnknown: return Theme.marquee
        default: return Theme.faint
        }
    }
}
