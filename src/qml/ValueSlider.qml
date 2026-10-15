import QtQuick
import QtQuick.Controls.Basic

// A slider with its value written beside it.
Row {
    id: root
    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize
    property alias value: slider.value
    property alias pressed: slider.pressed
    property var format: v => Math.round(v)
    signal moved(real value)
    spacing: 12

    Slider {
        id: slider
        width: 200
        anchors.verticalCenter: parent.verticalCenter
        snapMode: Slider.SnapAlways
        hoverEnabled: true
        onMoved: root.moved(value)
        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: slider.availableWidth
            height: 4
            radius: 2
            color: Theme.line
            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: Theme.blue
            }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 18; height: 18; radius: 9
            color: slider.pressed ? Theme.chalk : "#DDE3EC"
            border.width: slider.visualFocus ? 2 : 0
            border.color: Theme.blue
        }
    }
    Text {
        anchors.verticalCenter: parent.verticalCenter
        width: 56
        text: root.format(slider.value)
        color: Theme.dim
        font.family: Theme.family
        font.pixelSize: Theme.body
    }
}
