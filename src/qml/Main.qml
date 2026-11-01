// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    // What to show first (--view): home, play, saves, news (on Home), dx, settings/<page>.
    property string startView
    // The chosen game's entry ({gameId, title, state, logo, heroes, ...}).
    property var game: ({})
    property bool home: true  // Home, else the chosen game's page
    property bool dx: false   // GP DX's page (not OpenGP's game) instead of either
    property int tab: 0       // the game's page: 0 play, 1 saves & data
    property bool closeAfterBuild: false  // closed during an install or a build: once it has stopped
    // Room the update notice takes at the top right, which titles under it leave.
    readonly property real noticeSpace: updateNotice.visible ? updateNotice.width + 24 : 0

    visible: true
    minimumWidth: 1000
    minimumHeight: 640
    title: dx ? "OpenGP: Mario Kart Arcade GP DX" : !home && game.title ? "OpenGP: " + game.title : "OpenGP"
    color: Theme.asphalt
    font.family: Theme.family

    function updateGame() { game = Launcher.games.get(Launcher.currentIndex) }
    function addImage() { imageDialog.open() }
    function openGame(index) {
        Launcher.currentIndex = index
        home = false
        dx = false
    }

    Connections {
        target: Launcher.games
        function onDataChanged() { window.updateGame() }
        function onModelReset() { window.updateGame() }
    }
    Connections {
        target: Launcher
        function onCurrentChanged() { window.updateGame() }
        function onMessage(text, error) { toast.show(text, error) }
        function onMinimizeRequested() { window.showMinimized() }
        function onRaiseRequested() {
            if (window.visibility === Window.Minimized) window.showNormal()
            window.raise()
            window.requestActivate()
        }
        function onBuildingChanged() {
            if (window.closeAfterBuild && Launcher.buildingId.length === 0) window.close()
        }
    }
    // Closed during an install or a build: it is stopped first, with the
    // window still up, instead of the launcher carrying on without one.
    onClosing: close => {
        if (Launcher.buildingId.length === 0) return
        close.accepted = false
        closeAfterBuild = true
        Launcher.cancelBuild()
    }
    Component.onCompleted: {
        updateGame()
        if (startView === "play" || startView === "saves") home = false
        if (startView === "dx") dx = true
        if (startView === "saves") tab = 1
        else if (startView.startsWith("settings")) settings.openPage(startView.split("/")[1] || "")
    }

    // A disc image dropped on the window is added, as with Add disc image.
    DropArea {
        anchors.fill: parent
        enabled: Launcher.homeError.length === 0
        onDropped: drop => {
            if (drop.hasUrls && drop.urls.length > 0) {
                Launcher.addImage(drop.urls[0])
                drop.accept()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        GameRail {
            Layout.fillHeight: true
            Layout.preferredWidth: 96
            home: window.home && !window.dx
            dx: window.dx
            onOpenHome: { window.home = true; window.dx = false }
            onOpenDx: window.dx = true
            onOpenGame: index => window.openGame(index)
            onAddImage: window.addImage()
            onOpenSettings: settings.openPage("")
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            HeroPane {
                anchors.fill: parent
                game: window.dx ? ({}) : window.game
                dim: window.home ? 0.55 : window.tab === 1 ? 0.7 : 0
            }

            HomeView {
                anchors.fill: parent
                visible: Launcher.homeError.length === 0 && window.home && !window.dx
                onAddImage: window.addImage()
                onOpenGame: index => window.openGame(index)
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                visible: Launcher.homeError.length === 0 && !window.home && !window.dx

                // The game's logo, its name and the tabs.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 48
                    Layout.rightMargin: 48
                    Layout.topMargin: 36
                    spacing: 14

                    Item {
                        Layout.preferredHeight: 120
                        Layout.fillWidth: true
                        Image {
                            id: logo
                            source: window.game.logo || ""
                            visible: status === Image.Ready
                            height: 120
                            // Clear of the update notice, which is level with it.
                            width: Math.min(implicitWidth * height / Math.max(1, implicitHeight),
                                            parent.width - window.noticeSpace)
                            fillMode: Image.PreserveAspectFit
                            horizontalAlignment: Image.AlignLeft
                            smooth: true
                            mipmap: true
                        }
                        Text {
                            visible: !logo.visible
                            anchors.bottom: parent.bottom
                            text: window.game.title || ""
                            color: Theme.chalk
                            font.pixelSize: Theme.display + 6
                            font.weight: Font.ExtraBold
                        }
                    }

                    RowLayout {
                        spacing: 28
                        Repeater {
                            model: ["Play", "Saves & data"]
                            delegate: Item {
                                id: tabItem
                                required property int index
                                required property string modelData
                                implicitWidth: label.implicitWidth
                                implicitHeight: label.implicitHeight + 10
                                Text {
                                    id: label
                                    text: tabItem.modelData
                                    color: window.tab === tabItem.index ? Theme.chalk : Theme.dim
                                    font.pixelSize: Theme.lead + 1
                                    font.weight: window.tab === tabItem.index ? Font.ExtraBold : Font.Normal
                                }
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: 3
                                    radius: 2
                                    color: Theme.marquee
                                    visible: window.tab === tabItem.index
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: window.tab = tabItem.index
                                }
                            }
                        }
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: 20
                    currentIndex: window.tab
                    PlayTab {
                        game: window.game
                        onAddImage: window.addImage()
                    }
                    SavesTab {}
                }

                PlayBar {
                    Layout.fillWidth: true
                    game: window.game
                    onAddImage: window.addImage()
                }
            }

            DxView {
                anchors.fill: parent
                visible: Launcher.homeError.length === 0 && window.dx
                noticeSpace: window.noticeSpace
            }

            // An update, on every page, over what is there.
            UpdateNotice {
                id: updateNotice
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 40
                anchors.rightMargin: 48
            }

            // OpenGP is not where the launcher looked.
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 96, 620)
                visible: Launcher.homeError.length > 0
                spacing: 12
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: Launcher.homeErrorTitle || "OpenGP isn't in this folder"
                    color: Theme.chalk
                    font.pixelSize: Theme.heading
                    font.weight: Font.ExtraBold
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.dim
                    font.pixelSize: Theme.lead
                    lineHeight: 1.25
                    // Unpacked in the wrong place, opened from its zip, or
                    // a folder it can't write to: the reason says which.
                    text: Launcher.homeErrorTitle ? Launcher.homeError
                          : "Keep OpenGP (this program) in the folder it was unpacked to, next to its sdk folder. "
                            + "If you opened it from inside its zip file, unpack the whole archive first"
                            + (Qt.platform.os === "windows" ? " (right-click the zip and choose Extract All)" : "")
                            + ". The launcher looked in " + Launcher.homePath + ".\n\n"
                            + Launcher.homeError
                }
            }
        }
    }

    SettingsDialog { id: settings }
    Toast { id: toast }

    FileDialog {
        id: imageDialog
        title: "Add a disc image"
        // Compressed images too, so that choosing one says how to convert it.
        nameFilters: ["Disc images (*.iso *.gcm *.rvz *.gcz *.wia *.ciso)", "All files (*)"]
        onAccepted: Launcher.addImage(selectedFile)
    }
}
