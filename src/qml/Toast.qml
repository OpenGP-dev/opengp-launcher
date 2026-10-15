import QtQuick
import QtQuick.Controls.Basic

// A short message at the bottom right: what just happened, or what went wrong.
Popup {
    id: toast
    property bool error: false
    property string message

    function show(text, isError) {
        message = text
        error = isError
        open()
        hide.interval = isError ? 9000 : 4500
        hide.restart()
    }

    parent: Overlay.overlay
    x: parent.width - width - 24
    y: parent.height - height - 128
    width: Math.min(440, parent.width - 48)
    padding: 16
    leftPadding: 22
    closePolicy: Popup.CloseOnPressOutside
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200 } }

    background: Rectangle {
        color: Theme.panel
        radius: 12
        border.color: Theme.line
        Rectangle {
            width: 4
            radius: 2
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 10
            color: toast.error ? Theme.red : Theme.blue
        }
    }
    contentItem: Text {
        text: toast.message
        color: Theme.chalk
        font.family: Theme.family
        font.pixelSize: Theme.body
        wrapMode: Text.WordWrap
        lineHeight: 1.2
        MouseArea {
            anchors.fill: parent
            onClicked: toast.close()
        }
    }
    Timer {
        id: hide
        onTriggered: toast.close()
    }
}
