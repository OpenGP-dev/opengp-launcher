import QtQuick
import QtQuick.Effects
import QtQuick.Layouts

// Home and the games down the left, as on a game launcher; adding a game and
// the settings at the bottom.
Rectangle {
    id: rail
    property bool home: false  // Home is showing, not a game
    signal openHome()
    signal openGame(int index)
    signal addImage()
    signal openSettings()
    color: Theme.asphaltDeep

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 22
        anchors.bottomMargin: 18
        spacing: 18

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "OpenGP"
            color: Theme.chalk
            font.pixelSize: Theme.lead
            font.weight: Font.ExtraBold
        }

        Item {
            id: homeEntry
            Layout.fillWidth: true
            implicitHeight: 92

            Rectangle {
                width: 4
                height: 52
                radius: 2
                x: 0
                y: 6
                color: Theme.marquee
                visible: rail.home
            }
            Rectangle {
                id: homeTile
                width: 64; height: 64
                radius: 16
                anchors.horizontalCenter: parent.horizontalCenter
                color: homeArea.containsMouse ? Theme.panelHover : Theme.panel
                border.width: rail.home ? 2 : homeArea.containsMouse ? 1 : 0
                border.color: rail.home ? Theme.marquee : Theme.dim
                scale: homeArea.pressed ? 0.95 : 1
                Behavior on scale { NumberAnimation { duration: 90 } }
                Icon {
                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    name: "home"
                    color: rail.home ? Theme.chalk : Theme.dim
                }
            }
            Text {
                anchors.top: homeTile.bottom
                anchors.topMargin: 6
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Home"
                color: rail.home ? Theme.chalk : Theme.dim
                font.pixelSize: Theme.small
                font.weight: rail.home ? Font.ExtraBold : Font.Normal
            }
            MouseArea {
                id: homeArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: rail.openHome()
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14
            interactive: false
            model: Launcher.games
            currentIndex: Launcher.currentIndex
            delegate: Item {
                id: entry
                required property int index
                required property string shortTitle
                required property string title
                required property string tile
                required property string gameId
                required property int gameState
                readonly property bool chosen: !rail.home && Launcher.currentIndex === index
                readonly property bool playable: gameState === Launcher.Ready
                readonly property bool running: Launcher.runningId === gameId
                width: ListView.view.width
                height: 92

                Rectangle {
                    // The chosen game's mark.
                    width: 4
                    height: 52
                    radius: 2
                    x: 0
                    y: 6
                    color: Theme.marquee
                    visible: entry.chosen
                }

                Item {
                    id: tileBox
                    width: 64; height: 64
                    anchors.horizontalCenter: parent.horizontalCenter
                    opacity: entry.playable || entry.chosen ? 1 : 0.55
                    scale: hover.pressed ? 0.95 : 1
                    Behavior on scale { NumberAnimation { duration: 90 } }

                    Rectangle {
                        id: fallback
                        anchors.fill: parent
                        radius: 16
                        color: Theme.panel
                        Text {
                            // Until there is a picture; not under it, where its
                            // see-through parts would show the name.
                            visible: picture.status !== Image.Ready
                            anchors.centerIn: parent
                            text: entry.shortTitle
                            color: Theme.chalk
                            font.pixelSize: Theme.title
                            font.weight: Font.ExtraBold
                        }
                    }
                    Image {
                        id: picture
                        anchors.fill: parent
                        source: entry.tile
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        mipmap: true
                        // Drawn through the rounded mask below, unless there are no
                        // shader effects (software rendering): then square.
                        visible: GraphicsInfo.api === GraphicsInfo.Software && status === Image.Ready
                    }
                    Rectangle {
                        id: mask
                        anchors.fill: parent
                        radius: 16
                        visible: false
                        layer.enabled: true
                    }
                    MultiEffect {
                        anchors.fill: parent
                        source: picture
                        visible: picture.status === Image.Ready && GraphicsInfo.api !== GraphicsInfo.Software
                        maskEnabled: true
                        maskSource: mask
                        saturation: entry.playable || entry.chosen ? 0 : -0.8
                    }
                    Rectangle {
                        anchors.fill: parent
                        radius: 16
                        color: "transparent"
                        border.width: entry.chosen ? 2 : hover.containsMouse ? 1 : 0
                        border.color: entry.chosen ? Theme.marquee : Theme.dim
                    }
                    Rectangle {
                        // Running.
                        width: 14; height: 14; radius: 7
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: -3
                        color: Theme.red
                        border.width: 2
                        border.color: Theme.asphaltDeep
                        visible: entry.running
                    }
                }
                Text {
                    anchors.top: tileBox.bottom
                    anchors.topMargin: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: entry.shortTitle
                    color: entry.chosen ? Theme.chalk : Theme.dim
                    font.pixelSize: Theme.small
                    font.weight: entry.chosen ? Font.ExtraBold : Font.Normal
                }
                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: rail.openGame(entry.index)
                }
            }
        }

        // Neither means anything without OpenGP (no games): its settings are
        // kept in its folder.
        RailButton {
            visible: Launcher.games.count > 0
            icon: "plus"
            label: "Add game"
            onClicked: rail.addImage()
        }
        RailButton {
            visible: Launcher.games.count > 0
            icon: "gear"
            label: "Settings"
            onClicked: rail.openSettings()
        }
    }

    component RailButton: Item {
        id: button
        property string icon
        property string label
        signal clicked()
        Layout.fillWidth: true
        implicitHeight: 58
        Rectangle {
            id: circle
            width: 40; height: 40; radius: 20
            anchors.horizontalCenter: parent.horizontalCenter
            color: area.containsMouse ? Theme.panelHover : Theme.panel
            Icon {
                anchors.centerIn: parent
                width: 18
                height: 18
                name: button.icon
            }
        }
        Text {
            anchors.top: circle.bottom
            anchors.topMargin: 4
            anchors.horizontalCenter: parent.horizontalCenter
            text: button.label
            color: Theme.dim
            font.pixelSize: Theme.small - 1
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: button.clicked()
        }
    }
}
