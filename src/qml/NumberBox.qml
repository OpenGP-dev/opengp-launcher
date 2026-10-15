import QtQuick
import QtQuick.Controls.Basic

// A whole number typed in (window sizes, ports).
TextField {
    id: control
    property int from: 0
    property int to: 99999
    property int number: 0
    signal edited(int number)

    text: number
    implicitWidth: 96
    font.family: Theme.family
    font.pixelSize: Theme.body
    color: Theme.chalk
    selectionColor: Theme.blue
    horizontalAlignment: TextInput.AlignRight
    validator: IntValidator { bottom: 0; top: control.to }
    leftPadding: 10
    rightPadding: 10
    onEditingFinished: {
        const n = Math.max(from, Math.min(to, parseInt(text)))
        if (isNaN(n)) { text = number; return }
        text = n
        if (n !== number) edited(n)
    }
    background: Rectangle {
        implicitHeight: 34
        radius: 8
        color: Theme.panel
        border.width: 1
        border.color: control.activeFocus ? Theme.blue : Theme.line
    }
}
