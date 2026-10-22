import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The bottom of the window: the checkered band from the games' title
// screens, then the one button that does the next thing for the chosen game
// (add its disc image, install, update, play), and what is going on.
Rectangle {
    id: bar
    property var game: ({})
    signal addImage()

    readonly property int gameState: game.gameState === undefined ? Launcher.Checking : game.gameState
    readonly property bool building: Launcher.buildingId.length > 0 && Launcher.buildingId === game.gameId
    readonly property bool buildingOther: Launcher.buildingId.length > 0 && !building
    readonly property bool running: Launcher.runningId.length > 0
    readonly property bool runningThis: running && Launcher.runningId === game.gameId

    implicitHeight: 108
    color: Theme.asphaltDeep

    Canvas {
        id: flag
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 12
        onWidthChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            const size = 6
            ctx.clearRect(0, 0, width, height)
            for (let row = 0; row < 2; ++row)
                for (let x = 0, col = 0; x < width; x += size, ++col) {
                    ctx.fillStyle = (row + col) % 2 ? "#151A25" : "#D9DEE7"
                    ctx.fillRect(x, row * size, size, size)
                }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 12 + 16
        anchors.bottomMargin: 16
        anchors.leftMargin: 48
        anchors.rightMargin: 32
        spacing: 22

        // The button, with the run's options beside it.
        Row {
            spacing: 2
            GameButton {
                width: 232
                height: 60
                index: Launcher.currentIndex
                gameId: bar.game.gameId || ""
                gameState: bar.gameState
                joined: options.visible
                onAddImage: bar.addImage()
            }
            Button {
                id: options
                width: 40
                height: 60
                visible: bar.gameState === Launcher.Ready && !bar.running && !bar.building
                hoverEnabled: true
                onClicked: runMenu.open()
                contentItem: Item {
                    Icon {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        name: "chevron"
                        color: Theme.inkOnMarquee
                    }
                }
                background: Rectangle {
                    radius: 12
                    topLeftRadius: 4
                    bottomLeftRadius: 4
                    color: options.down ? Theme.marqueeDark : options.hovered ? "#FFD24F" : Theme.marquee
                }
                Menu {
                    id: runMenu
                    y: -height - 8
                    width: 300
                    background: Rectangle { color: Theme.panel; radius: 10; border.color: Theme.line }
                    RunItem { text: "Start in the test menu"; onTriggered: Launcher.play(Launcher.currentIndex, true, false) }
                    RunItem { text: "Play and log the game's own messages"; onTriggered: Launcher.play(Launcher.currentIndex, false, true) }
                }
            }
        }

        // What is going on.
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                Layout.fillWidth: true
                text: bar.statusTitle()
                color: Theme.chalk
                font.pixelSize: Theme.lead
                font.weight: Font.ExtraBold
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: !bar.building
                text: bar.statusText()
                color: Theme.dim
                font.pixelSize: Theme.body
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
            ProgressBar {
                id: progress
                Layout.fillWidth: true
                Layout.topMargin: 4
                Layout.bottomMargin: 4
                visible: bar.building
                from: 0
                to: 1
                value: Math.max(0, Launcher.progress)
                indeterminate: Launcher.progress < 0
                background: Rectangle { implicitHeight: 6; radius: 3; color: Theme.line }
                contentItem: Item {
                    implicitHeight: 6
                    Rectangle {
                        visible: !progress.indeterminate
                        width: progress.visualPosition * parent.width
                        height: parent.height
                        radius: 3
                        color: Theme.marquee
                    }
                    Rectangle {
                        id: runner
                        visible: progress.indeterminate
                        width: parent.width * 0.25
                        height: parent.height
                        radius: 3
                        color: Theme.marquee
                        XAnimator on x {
                            running: progress.indeterminate && progress.visible
                            from: -runner.width
                            to: progress.width
                            duration: 1400
                            loops: Animation.Infinite
                        }
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                visible: bar.building
                text: Launcher.progressText
                color: Theme.dim
                font.pixelSize: Theme.small + 1
                elide: Text.ElideMiddle
            }
        }

        // Beside the progress, not under it: the bar has room for three lines.
        PillButton {
            visible: bar.building
            text: "Stop"
            onClicked: Launcher.cancelBuild()
        }

        Text {
            Layout.alignment: Qt.AlignBottom | Qt.AlignRight
            horizontalAlignment: Text.AlignRight
            text: "Launcher " + Launcher.launcherVersion + "\nOpenGP " + Launcher.openGPVersion
            color: Theme.faint
            font.pixelSize: Theme.small
            lineHeight: 1.2
        }
    }

    component RunItem: MenuItem {
        id: item
        hoverEnabled: true
        contentItem: Text {
            text: item.text
            color: Theme.chalk
            font.family: Theme.family
            font.pixelSize: Theme.body
            leftPadding: 6
        }
        background: Rectangle { radius: 6; color: item.highlighted ? Theme.panelHover : "transparent" }
    }

    function statusTitle() {
        if (building) return "Building " + game.title
        if (runningThis) return "Playing " + game.title
        return game.title || ""
    }
    function statusText() {
        if (runningThis) return "Close the game to change its settings here. Until then, use its own menu (Esc)."
        if (running) return "Another game is running. Close it to play this one."
        switch (gameState) {
        case Launcher.Checking: return "Looking for the disc image and the game"
        case Launcher.NoImage:
            return game.imageNote || "OpenGP plays your own disc image of the game (.iso or .gcm). Add it here, or put it in the ISO folder next to OpenGP."
        case Launcher.NotBuilt:
            return Launcher.isPackage ? "Installing builds the game from your disc image. It takes a few minutes, once."
                                      : "Building runs CMake on this source tree with your disc image. The first build takes a while."
        case Launcher.OutOfDate: return "This game was built by another version of OpenGP. Updating builds it again."
        default: return "Ready to play"
        }
    }
}
