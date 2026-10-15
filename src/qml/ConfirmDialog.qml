import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// "Are you sure?" before something that can't be undone in one click.
Popup {
    id: dialog
    property string heading
    property string text
    property string confirmText: "OK"
    property bool dangerous: false
    property var action: null

    function ask(heading, text, confirmText, dangerous, action) {
        dialog.heading = heading
        dialog.text = text
        dialog.confirmText = confirmText
        dialog.dangerous = dangerous
        dialog.action = action
        open()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(460, parent.width - 64)
    modal: true
    focus: true
    padding: 24
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.55) }
    background: Rectangle { color: Theme.panel; radius: 14; border.color: Theme.line }

    contentItem: ColumnLayout {
        spacing: 14
        Text {
            Layout.fillWidth: true
            text: dialog.heading
            color: Theme.chalk
            font.family: Theme.family
            font.pixelSize: Theme.title
            font.weight: Font.ExtraBold
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            text: dialog.text
            color: Theme.dim
            font.family: Theme.family
            font.pixelSize: Theme.body
            wrapMode: Text.WordWrap
            lineHeight: 1.2
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            Layout.topMargin: 6
            spacing: 10
            PillButton {
                text: "Cancel"
                onClicked: dialog.close()
            }
            PillButton {
                text: dialog.confirmText
                kind: dialog.dangerous ? "danger" : "primary"
                onClicked: {
                    dialog.close()
                    if (dialog.action) dialog.action()
                }
            }
        }
    }
}
