import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// A short message at the bottom right: what just happened, or what went wrong.
Popup {
    id: toast
    property bool error: false
    property string message

    function show(text, isError) {
        message = text
        error = isError
        open()
        // An error stays until the player clicks it away: it may be the only
        // place they learn why a game didn't start.
        if (isError) {
            hide.stop()
        } else {
            hide.interval = 4500
            hide.restart()
        }
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
    // Long messages (a compiler's error) scroll instead of running off the
    // window, and can be selected and copied; the cross closes it.
    contentItem: RowLayout {
        spacing: 10
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(body.implicitHeight, toast.parent ? toast.parent.height * 0.6 : 400)
            clip: true
            TextArea {
                id: body
                text: toast.message
                readOnly: true
                selectByMouse: true
                wrapMode: Text.WordWrap
                color: Theme.chalk
                selectionColor: Theme.blue
                font.family: Theme.family
                font.pixelSize: Theme.body
                background: null
                padding: 0
            }
        }
        Text {
            Layout.alignment: Qt.AlignTop
            text: "\u2715"
            color: Theme.dim
            font.pixelSize: Theme.body
            MouseArea {
                anchors.fill: parent
                anchors.margins: -8
                cursorShape: Qt.PointingHandCursor
                onClicked: toast.close()
            }
        }
    }
    Timer {
        id: hide
        onTriggered: toast.close()
    }
}
