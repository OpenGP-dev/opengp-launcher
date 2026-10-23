// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic

Switch {
    id: control
    padding: 0
    hoverEnabled: true
    implicitWidth: 42
    implicitHeight: 24
    indicator: Rectangle {
        implicitWidth: 42
        implicitHeight: 24
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        radius: height / 2
        color: control.checked ? Theme.blue : Theme.line
        opacity: control.enabled ? 1 : 0.45
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.chalk
        Rectangle {
            width: 18; height: 18; radius: 9
            y: 3
            x: control.checked ? parent.width - width - 3 : 3
            color: Theme.chalk
            Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }
    }
    contentItem: Item {}
}
