import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control
    implicitWidth: 240
    font.family: Theme.family
    font.pixelSize: Theme.body
    color: Theme.chalk
    placeholderTextColor: Theme.faint
    selectionColor: Theme.blue
    leftPadding: 10
    rightPadding: 10
    background: Rectangle {
        implicitHeight: 34
        radius: 8
        color: Theme.panel
        border.width: 1
        border.color: control.activeFocus ? Theme.blue : Theme.line
    }
}
