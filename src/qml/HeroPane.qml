import QtQuick
import QtQuick.Effects

// Behind everything: the chosen game's course previews from the player's own
// disc, softened and darkened so text stays readable, changing slowly.
// Without them, asphalt.
Item {
    id: hero
    property var game: ({})
    readonly property var pictures: game.heroes || []
    property int shown: 0
    property real dim: 0  // 0 to 1: darker still
    clip: true

    onPicturesChanged: {
        shown = 0
        front.source = pictures.length ? pictures[0] : ""
        back.source = ""
        front.opacity = 1
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.asphalt
    }

    Item {
        id: art
        anchors.fill: parent
        layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 0.45
            blurMax: 40
            saturation: -0.05
            brightness: -0.08
        }
        Image {
            id: back
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
        }
        Image {
            id: front
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
            Behavior on opacity { NumberAnimation { duration: 1400; easing.type: Easing.InOutQuad } }
        }
    }

    // The next picture fades in over the last.
    Timer {
        interval: 9000
        repeat: true
        running: hero.pictures.length > 1 && Qt.application.state === Qt.ApplicationActive
        onTriggered: {
            hero.shown = (hero.shown + 1) % hero.pictures.length
            back.source = front.source
            front.opacity = 0
            front.source = hero.pictures[hero.shown]
            fadeIn.restart()
        }
    }
    Timer {
        id: fadeIn
        interval: 60
        onTriggered: front.opacity = 1
    }

    // Dark toward the left (the logo and the news) and the bottom (the play bar).
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(0.114, 0.137, 0.192, 0.94) }
            GradientStop { position: 0.55; color: Qt.rgba(0.114, 0.137, 0.192, 0.72) }
            GradientStop { position: 1.0; color: Qt.rgba(0.114, 0.137, 0.192, 0.38) }
        }
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: parent.height * 0.45
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Qt.rgba(0.082, 0.102, 0.145, 0.92) }
        }
    }
    Rectangle {
        anchors.fill: parent
        color: Theme.asphalt
        opacity: hero.dim
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }
}
