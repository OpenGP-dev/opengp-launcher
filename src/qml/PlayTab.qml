import QtQuick
import QtQuick.Layouts

// The news, and the chosen game's disc image and build.
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

        NewsPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: 720
        }

        // About this game's files.
        Rectangle {
            Layout.preferredWidth: 320
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
                        case Launcher.NotBuilt: return "Not installed yet."
                        case Launcher.NoImage: return "Needs its disc image."
                        default: return "Checking"
                        }
                    }
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: Launcher.isPackage
                    PillButton {
                        text: "Build again"
                        visible: tab.game.gameState === Launcher.Ready
                        enabled: Launcher.runningId.length === 0 && Launcher.buildingId.length === 0
                        onClicked: confirm.ask("Build " + tab.game.title + " again?",
                                               "OpenGP builds the game from your disc image again. It takes a few minutes.",
                                               "Build again", false, () => Launcher.build(Launcher.currentIndex))
                    }
                    PillButton {
                        text: "Uninstall"
                        visible: tab.game.gameState === Launcher.Ready || tab.game.gameState === Launcher.OutOfDate
                        enabled: Launcher.runningId.length === 0 && Launcher.buildingId.length === 0
                        onClicked: confirm.ask("Uninstall " + tab.game.title + "?",
                                               "This removes the built game. Your disc image, settings, cards and records stay.",
                                               "Uninstall", true, () => Launcher.removeBuild(Launcher.currentIndex))
                    }
                }
                PillButton {
                    text: "Open the OpenGP folder"
                    onClicked: Launcher.openFolder("home")
                }
            }
        }
    }

    ConfirmDialog { id: confirm }
}
