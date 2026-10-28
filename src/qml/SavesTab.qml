// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

// The chosen game's Mario Cards and its own saved data, backups of
// everything, screenshots and the last run's log.
Item {
    id: tab
    readonly property bool locked: UserData.locked
    property string selectedCard: ""
    property string selectedBackup: ""

    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.leftMargin: 48
        anchors.rightMargin: 24
        anchors.bottomMargin: 16
        contentWidth: availableWidth
        clip: true

        RowLayout {
            width: scroll.availableWidth - 24
            spacing: 32

            // Left: cards, the game's data, backups.
            ColumnLayout {
                Layout.preferredWidth: 1
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 28

                Section {
                    Layout.fillWidth: true
                    title: "Mario Cards"
                    note: tab.locked ? "Close the game to change cards."
                                     : "The card shown here is the card the game reads"

                    Repeater {
                        model: UserData.cards
                        delegate: Rectangle {
                            id: card
                            required property string modelData
                            readonly property bool held: GameSettings.card === modelData
                            readonly property bool selected: tab.selectedCard === modelData
                            Layout.fillWidth: true
                            implicitHeight: 48
                            radius: 10
                            color: selected ? Theme.panelHover : cardArea.containsMouse ? Theme.panel : Qt.rgba(0.153, 0.184, 0.255, 0.6)
                            border.color: selected ? Theme.blue : "transparent"
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 8
                                spacing: 10
                                Rectangle {
                                    width: 10; height: 10; radius: 5
                                    color: card.held ? Theme.marquee : "transparent"
                                    border.width: card.held ? 0 : 1
                                    border.color: Theme.faint
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: card.modelData
                                    color: Theme.chalk
                                    font.pixelSize: Theme.lead
                                    elide: Text.ElideRight
                                }
                                Text {
                                    visible: card.held
                                    text: "Using"
                                    color: Theme.marquee
                                    font.pixelSize: Theme.small
                                    font.weight: Font.ExtraBold
                                }
                                PillButton {
                                    visible: !card.held
                                    enabled: !tab.locked
                                    text: "Use"
                                    onClicked: GameSettings.card = card.modelData
                                }
                            }
                            MouseArea {
                                id: cardArea
                                anchors.fill: parent
                                z: -1
                                hoverEnabled: true
                                onClicked: tab.selectedCard = card.selected ? "" : card.modelData
                            }
                        }
                    }
                    Text {
                        visible: UserData.cards.length === 0
                        text: GameSettings.card.length ? "No cards created yet. " + GameSettings.card + " can be made in game."
                                                       : "No cards yet."
                        color: Theme.dim
                        font.pixelSize: Theme.body
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 8
                        PillButton { text: "New card"; enabled: !tab.locked; onClicked: UserData.newCard() }
                        PillButton { text: "Add a card file"; enabled: !tab.locked; onClicked: importDialog.open() }
                        PillButton {
                            text: "Rename"
                            visible: tab.selectedCard.length > 0
                            enabled: !tab.locked
                            onClicked: { renameBox.text = tab.selectedCard; rename.open() }
                        }
                        PillButton {
                            text: "Copy"
                            visible: tab.selectedCard.length > 0
                            enabled: !tab.locked
                            onClicked: UserData.duplicateCard(tab.selectedCard)
                        }
                        PillButton {
                            text: "Export"
                            visible: tab.selectedCard.length > 0
                            onClicked: exportDialog.open()
                        }
                        PillButton {
                            text: "Delete"
                            visible: tab.selectedCard.length > 0
                            enabled: !tab.locked
                            onClicked: confirm.ask("Delete " + tab.selectedCard + "?",
                                                   "Once deleted, unless you have a backup, the card cannot be recovered.",
                                                   "Delete card", true, () => { UserData.deleteCard(tab.selectedCard); tab.selectedCard = "" })
                        }
                    }
                }

                Section {
                    Layout.fillWidth: true
                    title: "Game Data"
                    note: "Wipe all local cabinet data. and reset to default. (Cards are unaffected)"
                    PillButton {
                        text: "Factory Reset"
                        kind: "danger"
                        enabled: !tab.locked && UserData.hasGameData
                        onClicked: confirm.ask("Reset the game's data?",
                                               "The game starts as a new cabinet: test-menu settings, records and rankings are reset "
                                               + "A backup of everything is made first.",
                                               "Reset", true, () => UserData.resetGameData())
                    }
                }

                Section {
                    Layout.fillWidth: true
                    title: "Backups"
                    note: "Backups copy your settings, controls, cards and data across both games. "
                          + "Restoring from a backup will automatically make a backup of your current settings"

                    Repeater {
                        model: UserData.backups
                        delegate: Rectangle {
                            id: backup
                            required property var modelData
                            readonly property bool selected: tab.selectedBackup === modelData.name
                            Layout.fillWidth: true
                            implicitHeight: 40
                            radius: 8
                            color: selected ? Theme.panelHover : backupArea.containsMouse ? Theme.panel : "transparent"
                            border.color: selected ? Theme.blue : "transparent"
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                Text {
                                    Layout.fillWidth: true
                                    text: backup.modelData.name
                                    color: Theme.chalk
                                    font.pixelSize: Theme.body
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: backup.modelData.size
                                    color: Theme.dim
                                    font.pixelSize: Theme.small
                                }
                            }
                            MouseArea {
                                id: backupArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: tab.selectedBackup = backup.selected ? "" : backup.modelData.name
                            }
                        }
                    }
                    Text {
                        visible: UserData.backups.length === 0
                        text: "No backups yet."
                        color: Theme.dim
                        font.pixelSize: Theme.body
                    }
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 8
                        PillButton { text: "Back up"; onClicked: UserData.backUp() }
                        PillButton {
                            text: "Restore"
                            enabled: !tab.locked && tab.selectedBackup.length > 0
                            onClicked: confirm.ask("Restore " + tab.selectedBackup + "?",
                                                   "Settings, controls, cards and game data are restored to the time of the backup.",
                                                   "Restore", false, () => UserData.restoreBackup(tab.selectedBackup))
                        }
                        PillButton {
                            text: "Delete"
                            enabled: tab.selectedBackup.length > 0
                            onClicked: confirm.ask("Delete this backup?", tab.selectedBackup, "Delete backup", true,
                                                   () => { UserData.deleteBackup(tab.selectedBackup); tab.selectedBackup = "" })
                        }
                        PillButton { text: "Open folder"; onClicked: Launcher.openFolder("backups") }
                    }
                }
            }

            // Right: screenshots and the log.
            ColumnLayout {
                Layout.preferredWidth: 1
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 28

                Section {
                    Layout.fillWidth: true
                    title: "Screenshots"
                    note: UserData.screenshots.length ? "" : "Press F12 at any time to take a screenshot"
                    Grid {
                        id: shots
                        Layout.fillWidth: true
                        columns: Math.max(1, Math.floor(width / 160))
                        spacing: 8
                        Repeater {
                            model: UserData.screenshots.slice(0, 12)
                            delegate: Image {
                                required property string modelData
                                width: (shots.width - (shots.columns - 1) * shots.spacing) / shots.columns
                                height: width * 0.75
                                source: modelData
                                sourceSize.width: 320
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Qt.openUrlExternally(parent.modelData)
                                }
                            }
                        }
                    }
                    PillButton {
                        text: "Open folder"
                        onClicked: Launcher.openFolder("screenshots")
                    }
                }

                Section {
                    Layout.fillWidth: true
                    title: "Last run's log"
                    note: UserData.log.length ? "" : "The game's internal logs are found here (useful for debugging)"
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 220
                        visible: UserData.log.length > 0
                        radius: 10
                        color: Theme.asphaltDeep
                        border.color: Theme.line
                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 10
                            clip: true
                            TextArea {
                                text: UserData.log
                                readOnly: true
                                wrapMode: TextEdit.NoWrap
                                color: "#C9D0DC"
                                selectionColor: Theme.blue
                                font.family: "Consolas, Menlo, monospace"
                                font.pixelSize: Theme.small
                                background: null
                                Component.onCompleted: cursorPosition = length
                            }
                        }
                    }
                    PillButton {
                        text: "Open logs folder"
                        onClicked: Launcher.openFolder("logs")
                    }
                }
            }
        }
    }

    ConfirmDialog { id: confirm }

    Popup {
        id: rename
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        focus: true
        padding: 24
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.55) }
        background: Rectangle { color: Theme.panel; radius: 14; border.color: Theme.line }
        onOpened: { renameBox.forceActiveFocus(); renameBox.selectAll() }
        contentItem: ColumnLayout {
            spacing: 14
            Text {
                text: "Rename " + tab.selectedCard
                color: Theme.chalk
                font.family: Theme.family
                font.pixelSize: Theme.title
                font.weight: Font.ExtraBold
            }
            TextBox {
                id: renameBox
                Layout.preferredWidth: 320
                onAccepted: done.clicked()
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                PillButton { text: "Cancel"; onClicked: rename.close() }
                PillButton {
                    id: done
                    text: "Rename"
                    kind: "primary"
                    enabled: renameBox.text.trim().length > 0
                    onClicked: {
                        if (UserData.renameCard(tab.selectedCard, renameBox.text)) tab.selectedCard = renameBox.text.trim()
                        rename.close()
                    }
                }
            }
        }
    }

    FileDialog {
        id: importDialog
        title: "Add a Mario Card file"
        nameFilters: ["Mario Cards (*.bin)", "All files (*)"]
        onAccepted: UserData.importCard(selectedFile)
    }
    FileDialog {
        id: exportDialog
        title: "Save the card as a file"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Mario Cards (*.bin)"]
        onAccepted: UserData.exportCard(tab.selectedCard, selectedFile)
    }
}
