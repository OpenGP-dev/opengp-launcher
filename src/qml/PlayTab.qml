import QtQuick
import QtQuick.Layouts

// The chosen game's disc image and build (the news is on Home).
Item {
    id: tab
    property var game: ({})
    signal addImage()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 48
        anchors.rightMargin: 48
        anchors.bottomMargin: 24
        spacing: 32

        // About this game's files.
        Rectangle {
            Layout.preferredWidth: 380
            Layout.alignment: Qt.AlignTop
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
                    text: tab.game.image ? tab.game.image.split(/[\\/]/).pop() : "None yet"
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
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    PillButton {
                        text: tab.game.image ? "Change" : "Add disc image"
                        enabled: Launcher.runningId.length === 0 && Launcher.buildingId.length === 0
                        onClicked: tab.addImage()
                    }
                    PillButton {
                        text: "Show file"
                        visible: !!tab.game.image
                        onClicked: Launcher.openFolder("image")
                    }
                    PillButton {
                        text: "Check again"
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
                        case Launcher.Ready: return Launcher.isPackage ? "Installed, built by this version of OpenGP." : "Built from the OpenGP source tree."
                        case Launcher.OutOfDate: return "Built by another version of OpenGP."
                        case Launcher.NotBuilt: return Launcher.isPackage ? "Not installed yet." : "Not built yet."
                        case Launcher.NoImage: return "Needs its disc image."
                        default: return "Checking"
                        }
                    }
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    PillButton {
                        text: "Build again"
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
                        text: "Open the OpenGP folder"
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
        case Launcher.DumpGood: return "A known good dump"
        case Launcher.DumpKnownBad: return "A known bad dump"
        case Launcher.DumpUnknown: return "Not a known good dump"
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
