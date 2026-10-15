import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The games' settings (shared by both games) and the launcher's own.
Popup {
    id: dialog
    readonly property var pages: ["Video", "Sound", "Game", "Camera", "Linked cabinets", "Controls", "Launcher"]
    property int page: 0
    readonly property bool locked: GameSettings.locked

    function openPage(name) {
        const i = pages.findIndex(p => p.toLowerCase().replace(/ /g, "-") === name.toLowerCase())
        if (i >= 0) page = i
        open()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(980, parent.width - 64)
    height: Math.min(700, parent.height - 64)
    modal: true
    focus: true
    padding: 0
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.6) }
    background: Rectangle { color: Theme.asphalt; radius: 16; border.color: Theme.line }
    onClosed: Controls.cancelCapture()

    contentItem: RowLayout {
        spacing: 0

        // The pages.
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 220
            color: Theme.asphaltDeep
            radius: 16
            Rectangle { anchors.right: parent.right; width: 16; height: parent.height; color: parent.color }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 4
                Text {
                    text: "Settings"
                    color: Theme.chalk
                    font.family: Theme.family
                    font.pixelSize: Theme.heading
                    font.weight: Font.ExtraBold
                    Layout.bottomMargin: 14
                    Layout.leftMargin: 10
                }
                Repeater {
                    model: dialog.pages
                    delegate: Rectangle {
                        id: navItem
                        required property int index
                        required property string modelData
                        Layout.fillWidth: true
                        implicitHeight: 40
                        radius: 10
                        color: dialog.page === index ? Theme.panel : navArea.containsMouse ? Qt.rgba(0.153, 0.184, 0.255, 0.5) : "transparent"
                        Rectangle {
                            width: 3; height: 18; radius: 2
                            anchors.verticalCenter: parent.verticalCenter
                            x: 0
                            color: Theme.marquee
                            visible: dialog.page === navItem.index
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 14
                            text: navItem.modelData
                            color: dialog.page === navItem.index ? Theme.chalk : Theme.dim
                            font.family: Theme.family
                            font.pixelSize: Theme.lead
                            font.weight: dialog.page === navItem.index ? Font.ExtraBold : Font.Normal
                        }
                        MouseArea {
                            id: navArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { Controls.cancelCapture(); dialog.page = navItem.index }
                        }
                    }
                }
                Item { Layout.fillHeight: true }
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    text: "Settings are shared by both games."
                    color: Theme.faint
                    font.family: Theme.family
                    font.pixelSize: Theme.small
                    wrapMode: Text.WordWrap
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 36
                Layout.rightMargin: 16
                Layout.topMargin: 16
                Text {
                    Layout.fillWidth: true
                    text: dialog.pages[dialog.page]
                    color: Theme.chalk
                    font.family: Theme.family
                    font.pixelSize: Theme.heading
                    font.weight: Font.ExtraBold
                }
                PillButton {
                    text: "Done"
                    kind: "primary"
                    onClicked: dialog.close()
                }
            }

            // Why the settings can't change now.
            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 36
                Layout.rightMargin: 36
                Layout.topMargin: 14
                visible: dialog.locked || GameSettings.error.length > 0
                implicitHeight: notice.implicitHeight + 24
                radius: 10
                color: Qt.rgba(0.898, 0.141, 0.231, 0.14)
                border.color: Qt.rgba(0.898, 0.141, 0.231, 0.5)
                RowLayout {
                    id: notice
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: Theme.chalk
                        font.family: Theme.family
                        font.pixelSize: Theme.body
                        text: dialog.locked
                              ? "A game is running. Change its settings in its own menu (Esc) until you close it."
                              : "The settings file can't be read, so the games are using their defaults: " + GameSettings.error
                    }
                    PillButton {
                        visible: !dialog.locked
                        text: "Reset to defaults"
                        onClicked: GameSettings.resetToDefaults()
                    }
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 8
                currentIndex: dialog.page

                // Video
                Page {
                    enabled: !dialog.locked
                    SettingRow {
                        label: "Full screen"
                        help: "F11 or Alt+Enter switches in the game too."
                        Toggle { checked: GameSettings.fullscreen; onToggled: GameSettings.fullscreen = checked }
                    }
                    SettingRow {
                        label: "Window size"
                        help: "When not full screen. The game remembers it when you resize its window."
                        Row {
                            spacing: 8
                            NumberBox { from: 320; to: 16384; number: GameSettings.windowWidth; onEdited: n => GameSettings.windowWidth = n }
                            Text { text: "×"; color: Theme.dim; font.pixelSize: Theme.lead; anchors.verticalCenter: parent.verticalCenter }
                            NumberBox { from: 240; to: 16384; number: GameSettings.windowHeight; onEdited: n => GameSettings.windowHeight = n }
                        }
                    }
                    SettingRow {
                        label: "Shape of the picture"
                        help: "The games were made for 4:3. Wider shapes show more of each course."
                        Choice {
                            model: ["The window's shape", "4:3, as in the arcade", "16:9", "16:10", "21:9"]
                            currentIndex: GameSettings.aspect
                            onActivated: i => GameSettings.aspect = i
                        }
                    }
                    SettingRow {
                        label: "Resolution"
                        help: "How sharply the game draws, as a multiple of the cabinet's 640×528."
                        Choice {
                            model: ["Match the window", "1× (as in the arcade)", "2×", "3×", "4×", "5×", "6×", "7×", "8×"]
                            currentIndex: GameSettings.internalResolution
                            onActivated: i => GameSettings.internalResolution = i
                        }
                    }
                    SettingRow {
                        label: "VSync"
                        help: "Waits for the screen, so the picture never tears."
                        Toggle { checked: GameSettings.vsync; onToggled: GameSettings.vsync = checked }
                    }
                }

                // Sound
                Page {
                    enabled: !dialog.locked
                    SettingRow {
                        label: "Volume"
                        ValueSlider {
                            from: 0; to: 100; stepSize: 1
                            value: GameSettings.volume
                            format: v => Math.round(v) + "%"
                            onMoved: v => GameSettings.volume = Math.round(v)
                        }
                    }
                }

                // Game
                Page {
                    enabled: !dialog.locked
                    SettingRow {
                        label: "Free play"
                        help: "Races without coins."
                        Toggle { checked: GameSettings.freePlay; onToggled: GameSettings.freePlay = checked }
                    }
                    SettingRow {
                        label: "Pause while the menu is open"
                        help: "The game's own menu (Esc). Never in linked races."
                        Toggle { checked: GameSettings.pauseInMenu; onToggled: GameSettings.pauseInMenu = checked }
                    }
                    SettingRow {
                        label: "Pause in the background"
                        help: "When you switch to another window."
                        Toggle { checked: GameSettings.pauseInBackground; onToggled: GameSettings.pauseInBackground = checked }
                    }
                    SettingRow {
                        label: "Show what you're playing on Discord"
                        help: "Your profile shows the game, the course and your place while Discord runs on this computer."
                        Toggle { checked: GameSettings.richPresence; onToggled: GameSettings.richPresence = checked }
                    }
                    SettingRow {
                        label: "Mario Card"
                        help: "Choose and look after the cards under Saves & data."
                        Text {
                            text: GameSettings.card.length ? GameSettings.card : "None"
                            color: Theme.chalk
                            font.family: Theme.family
                            font.pixelSize: Theme.lead
                        }
                    }
                }

                // Camera
                Page {
                    enabled: !dialog.locked
                    SettingRow {
                        label: "Photos from"
                        help: "The cabinet's camera takes the players' photos for their cards. Changes apply when the game starts."
                        Choice {
                            model: ["No camera", "A webcam", "Colour bars (test)"]
                            currentIndex: GameSettings.camera
                            onActivated: i => GameSettings.camera = i
                        }
                    }
                    SettingRow {
                        label: "Webcam"
                        visible: GameSettings.camera === 1
                        Choice {
                            id: webcamChoice
                            property var cams: []
                            model: ["The first one"].concat(cams)
                            currentIndex: Math.max(0, cams.indexOf(GameSettings.webcam) + 1)
                            onActivated: i => GameSettings.webcam = i === 0 ? "" : cams[i - 1]
                            Component.onCompleted: if (GameSettings.camera === 1) cams = GameSettings.webcams()
                            Connections {
                                target: dialog
                                function onPageChanged() {
                                    if (dialog.page === 3 && GameSettings.camera === 1) webcamChoice.cams = GameSettings.webcams()
                                }
                            }
                        }
                    }
                }

                // Linked cabinets
                Page {
                    enabled: !dialog.locked
                    note: "Up to four cabinets link for Versus races: copies of OpenGP on this computer, or on computers on your network. "
                          + "Give each its own number. These apply when the game starts."
                    SettingRow {
                        label: "Cabinet number"
                        Choice {
                            model: ["The game's own (test menu)", "1", "2", "3", "4"]
                            currentIndex: GameSettings.cabinet
                            onActivated: i => GameSettings.cabinet = i
                        }
                    }
                    SettingRow {
                        label: "Link over the local network"
                        help: "Also finds cabinets on other computers on your network."
                        Toggle { checked: GameSettings.lan; onToggled: GameSettings.lan = checked }
                    }
                    SettingRow {
                        label: "First port"
                        help: "The link uses this port and the 39 after it. Every cabinet must use the same."
                        NumberBox { from: 1024; to: 65495; number: GameSettings.linkPort; onEdited: n => GameSettings.linkPort = n }
                    }
                }

                // Controls
                ControlsPage {}

                // Launcher
                Page {
                    SettingRow {
                        label: "When a game starts"
                        Choice {
                            model: ["Keep the launcher open", "Minimize the launcher", "Close the launcher"]
                            currentIndex: Launcher.onLaunch
                            onActivated: i => Launcher.onLaunch = i
                        }
                    }
                    SettingRow {
                        label: "News"
                        help: "Announcements from the OpenGP team, downloaded when the launcher starts."
                        Toggle { checked: News.enabled; onToggled: News.enabled = checked }
                    }
                    SettingRow {
                        label: "OpenGP's folder"
                        help: Launcher.homePath
                        PillButton { text: "Open"; onClicked: Launcher.openFolder("home") }
                    }
                    SettingRow {
                        label: "Settings and saves"
                        help: Launcher.userPath
                        PillButton { text: "Open"; onClicked: Launcher.openFolder("user") }
                    }
                    SettingRow {
                        label: "Versions"
                        help: "Launcher " + Launcher.launcherVersion + ", OpenGP " + Launcher.openGPVersion
                    }
                }
            }
        }
    }

    // A page of settings that scrolls.
    component Page: ScrollView {
        id: pageView
        property string note
        default property alias rows: column.data
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            id: column
            x: 36
            width: pageView.availableWidth - 72
            spacing: 10
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                visible: pageView.note.length > 0
                text: pageView.note
                color: Theme.dim
                font.family: Theme.family
                font.pixelSize: Theme.body
                wrapMode: Text.WordWrap
                lineHeight: 1.2
            }
        }
    }
}
