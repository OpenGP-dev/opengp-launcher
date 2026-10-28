// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic

// A button: "primary" (marquee yellow), "quiet" (outline) or "danger".
Button {
    id: control
    property string kind: "quiet"
    property color fill: kind === "primary" ? Theme.marquee : kind === "danger" ? Theme.red : "transparent"

    font.family: Theme.family
    font.pixelSize: Theme.body
    font.weight: kind === "primary" ? Font.ExtraBold : Font.Normal
    padding: 8
    leftPadding: 16
    rightPadding: 16
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.kind === "primary" ? Theme.inkOnMarquee : Theme.chalk
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        implicitHeight: 34
        radius: height / 2
        color: control.kind === "quiet"
               ? (control.down ? Theme.panelHover : control.hovered ? Theme.panel : "transparent")
               : (control.down ? Qt.darker(control.fill, 1.15) : control.hovered ? Qt.lighter(control.fill, 1.08) : control.fill)
        border.width: control.kind === "quiet" || control.visualFocus ? 1 : 0
        border.color: control.visualFocus ? Theme.blue : Theme.line
    }
}
