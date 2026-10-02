import QtQuick
import QtQuick.Layouts

// Home: each game with the one thing to do next (a card opens the game's own
// page), then the news.
Item {
    id: home
    signal addImage()
    signal openGame(int index)

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 48
        anchors.rightMargin: 48
        anchors.topMargin: 36
        anchors.bottomMargin: 24
        spacing: 24

        Text {
            text: "OpenGP"
            color: Theme.chalk
            font.pixelSize: Theme.display + 6
            font.weight: Font.ExtraBold
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20
            Repeater {
                model: Launcher.games
                delegate: GameCard {}
            }
            Item { Layout.fillWidth: true }
        }

        NewsPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: 940
        }
    }

    component GameCard: Rectangle {
        id: card
        required property int index
        required property string gameId
        required property string title
        required property string logo
        required property int gameState
        readonly property bool building: gameId.length > 0 && Launcher.buildingId === gameId
        readonly property bool runningThis: gameId.length > 0 && Launcher.runningId === gameId

        Layout.fillWidth: true
        Layout.maximumWidth: 460
        implicitHeight: 176
        radius: 14
        color: area.containsMouse ? Qt.rgba(0.153, 0.184, 0.255, 0.86) : Qt.rgba(0.082, 0.102, 0.145, 0.78)
        border.color: Theme.line

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: home.openGame(card.index)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 8

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                Image {
                    id: logo
                    source: card.logo
                    visible: status === Image.Ready
                    height: parent.height
                    width: Math.min(implicitWidth * height / Math.max(1, implicitHeight), parent.width)
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignLeft
                    smooth: true
                    mipmap: true
                }
                Text {
                    visible: !logo.visible
                    width: parent.width
                    anchors.verticalCenter: parent.verticalCenter
                    text: card.title
                    color: Theme.chalk
                    font.pixelSize: Theme.title
                    font.weight: Font.ExtraBold
                    elide: Text.ElideRight
                }
            }
            Text {
                Layout.fillWidth: true
                text: card.statusText()
                color: Theme.dim
                font.pixelSize: Theme.body
                elide: Text.ElideRight
            }
            Item { Layout.fillHeight: true }
            Row {
                spacing: 10
                GameButton {
                    width: 180
                    height: 44
                    font.pixelSize: 18
                    index: card.index
                    gameId: card.gameId
                    gameState: card.gameState
                    onAddImage: home.addImage()
                }
                PillButton {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: card.building
                    text: "Stop"
                    onClicked: Launcher.cancelBuild()
                }
            }
        }

        function statusText() {
            if (building) return Launcher.progress >= 0 ? "Building: " + Math.round(Launcher.progress * 100) + "%" : "Building"
            if (runningThis) return "Playing"
            switch (gameState) {
            case Launcher.Checking: return "Checking"
            case Launcher.NoImage: return "Needs its disc image"
            case Launcher.NotBuilt: return Launcher.isPackage ? "Not installed yet" : "Not built yet"
            case Launcher.OutOfDate: return "Built by another version of OpenGP"
            default: return "Ready to play"
            }
        }
    }
}
