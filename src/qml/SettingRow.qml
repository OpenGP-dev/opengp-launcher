// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

// One setting: what it is (and what it does) on the left, its control on the right.
RowLayout {
    id: root
    property string label
    property string help
    default property alias control: holder.data
    Layout.fillWidth: true
    Layout.minimumHeight: 48
    spacing: 24

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        Text {
            text: root.label
            color: Theme.chalk
            font.family: Theme.family
            font.pixelSize: Theme.lead
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Text {
            text: root.help
            visible: text.length > 0
            color: Theme.dim
            font.family: Theme.family
            font.pixelSize: Theme.small + 1
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
    }
    Row {
        id: holder
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }
}
