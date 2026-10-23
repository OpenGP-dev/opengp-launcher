// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic

// A drop-down list.
ComboBox {
    id: control
    font.family: Theme.family
    font.pixelSize: Theme.body
    hoverEnabled: true
    implicitWidth: 220
    opacity: enabled ? 1 : 0.45

    contentItem: Text {
        leftPadding: 12
        rightPadding: control.indicator.width + 8
        text: control.displayText
        font: control.font
        color: Theme.chalk
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Icon {
        x: control.width - width - 12
        anchors.verticalCenter: parent.verticalCenter
        width: 14
        height: 14
        name: "chevron"
        color: Theme.dim
    }
    background: Rectangle {
        implicitHeight: 34
        radius: 8
        color: control.hovered ? Theme.panelHover : Theme.panel
        border.width: 1
        border.color: control.visualFocus || control.popup.visible ? Theme.blue : Theme.line
    }
    delegate: ItemDelegate {
        id: item
        required property int index
        required property var modelData
        width: control.width
        hoverEnabled: true
        highlighted: control.highlightedIndex === index
        contentItem: Text {
            text: control.textRole ? item.modelData[control.textRole] : item.modelData
            font: control.font
            color: control.currentIndex === item.index ? Theme.marquee : Theme.chalk
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle { color: item.highlighted || item.hovered ? Theme.panelHover : "transparent" }
    }
    popup: Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 320)
        padding: 4
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
        }
        background: Rectangle { color: Theme.panel; radius: 8; border.color: Theme.line }
    }
}
