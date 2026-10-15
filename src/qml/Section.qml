import QtQuick
import QtQuick.Layouts

// A group of settings or data under a heading, with an optional note.
ColumnLayout {
    id: root
    property string title
    property string note
    default property alias content: body.data
    spacing: 10

    Text {
        text: root.title
        visible: text.length > 0
        color: Theme.chalk
        font.family: Theme.family
        font.pixelSize: Theme.title
        font.weight: Font.ExtraBold
    }
    Text {
        text: root.note
        visible: text.length > 0
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: Theme.dim
        font.family: Theme.family
        font.pixelSize: Theme.body
        lineHeight: 1.2
    }
    ColumnLayout {
        id: body
        Layout.fillWidth: true
        spacing: 4
    }
}
