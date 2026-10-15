import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// What drives the cabinet's controls: keys, gamepad buttons and sticks, and
// wheels and pedals (other controllers). Every connected device works at once.
ScrollView {
    id: page
    contentWidth: availableWidth
    clip: true
    readonly property var devices: ["Keyboard", "Gamepad", "Wheel and other controllers"]

    ColumnLayout {
        x: 36
        width: page.availableWidth - 72
        spacing: 14

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 8
            visible: Controls.locked
            text: "A game is running and has the controllers. Close it to change the controls."
            color: Theme.dim
            font.family: Theme.family
            font.pixelSize: Theme.body
            wrapMode: Text.WordWrap
        }

        // Which device.
        Row {
            spacing: 6
            Repeater {
                model: page.devices
                delegate: Rectangle {
                    id: deviceTab
                    required property int index
                    required property string modelData
                    width: deviceLabel.implicitWidth + 32
                    height: 36
                    radius: 18
                    color: Controls.device === index ? Theme.chalk : deviceArea.containsMouse ? Theme.panelHover : Theme.panel
                    Text {
                        id: deviceLabel
                        anchors.centerIn: parent
                        text: deviceTab.modelData
                        color: Controls.device === deviceTab.index ? Theme.asphalt : Theme.chalk
                        font.family: Theme.family
                        font.pixelSize: Theme.body
                        font.weight: Controls.device === deviceTab.index ? Font.ExtraBold : Font.Normal
                    }
                    MouseArea {
                        id: deviceArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Controls.device = deviceTab.index
                    }
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: Controls.device > 0
            text: Controls.connected.length ? "Connected: " + Controls.connected.join(", ") : "No controllers connected."
            color: Theme.dim
            font.family: Theme.family
            font.pixelSize: Theme.small + 1
            wrapMode: Text.WordWrap
        }

        // The cabinet's controls and what is bound to each.
        Repeater {
            model: Controls.rows
            delegate: RowLayout {
                id: row
                required property var modelData
                readonly property bool capturing: Controls.capturing === modelData.control
                Layout.fillWidth: true
                Layout.minimumHeight: 44
                spacing: 16

                Text {
                    Layout.preferredWidth: 150
                    text: row.modelData.label
                    color: Theme.chalk
                    font.family: Theme.family
                    font.pixelSize: Theme.lead
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: row.modelData.bindings
                        delegate: Rectangle {
                            id: chip
                            required property int index
                            required property string modelData
                            height: 30
                            width: chipText.implicitWidth + remove.width + 26
                            radius: 15
                            color: Theme.panel
                            border.color: Theme.line
                            Text {
                                id: chipText
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: chip.modelData
                                color: Theme.chalk
                                font.family: Theme.family
                                font.pixelSize: Theme.body
                            }
                            Icon {
                                id: remove
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                width: 12
                                height: 12
                                name: "cross"
                                color: removeArea.containsMouse ? Theme.red : Theme.faint
                                MouseArea {
                                    id: removeArea
                                    anchors.fill: parent
                                    anchors.margins: -6
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Controls.removeBinding(row.modelData.control, chip.index)
                                }
                            }
                        }
                    }
                    Rectangle {
                        // Waiting for the input to bind.
                        visible: row.capturing
                        height: 30
                        width: waiting.implicitWidth + 24
                        radius: 15
                        color: "transparent"
                        border.color: Theme.marquee
                        Text {
                            id: waiting
                            anchors.centerIn: parent
                            text: Controls.device === 0 ? "Press a key (Esc to stop)"
                                : row.modelData.axis ? "Move a stick or wheel all the way (Esc to stop)"
                                : "Press a button or move a stick (Esc to stop)"
                            color: Theme.marquee
                            font.family: Theme.family
                            font.pixelSize: Theme.body
                        }
                        SequentialAnimation on opacity {
                            running: row.capturing
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.45; duration: 600 }
                            NumberAnimation { to: 1; duration: 600 }
                        }
                    }
                    PillButton {
                        visible: !row.capturing
                        height: 30
                        text: "Add"
                        enabled: !Controls.locked && Controls.capturing < 0
                        onClicked: Controls.startCapture(row.modelData.control)
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line; Layout.topMargin: 6 }

        SettingRow {
            label: "Steering speed"
            help: "How fast keys turn the wheel: full turns a second."
            visible: Controls.device === 0
            ValueSlider {
                from: 0.5; to: 10; stepSize: 0.5
                value: Controls.steerSpeed
                format: v => v.toFixed(1)
                onMoved: v => Controls.steerSpeed = v
            }
        }
        SettingRow {
            label: "Dead zone"
            help: "How far a stick or wheel moves before the game sees it."
            visible: Controls.device > 0
            ValueSlider {
                from: 0; to: 0.5; stepSize: 0.01
                value: Controls.deadzone
                format: v => Math.round(v * 100) + "%"
                onMoved: v => Controls.deadzone = v
            }
        }
        SettingRow {
            label: "Force feedback"
            help: "How hard wheels push back, and how strongly gamepads rumble."
            visible: Controls.device > 0
            ValueSlider {
                from: 0; to: 2; stepSize: 0.05
                value: Controls.forceFeedback
                format: v => Math.round(v * 100) + "%"
                onMoved: v => Controls.forceFeedback = v
            }
        }
        SettingRow {
            label: "Use the controls in the background too"
            help: "Every OpenGP window reads the controls, active or not, to drive several cabinets on one computer."
            Toggle { checked: Controls.backgroundInput; onToggled: Controls.backgroundInput = checked }
        }
        Text {
            Layout.fillWidth: true
            visible: Controls.error.length > 0
            text: Controls.error
            color: Theme.red
            font.family: Theme.family
            font.pixelSize: Theme.body
            wrapMode: Text.WordWrap
        }
        PillButton {
            Layout.topMargin: 6
            Layout.bottomMargin: 24
            text: "Restore the defaults"
            enabled: !Controls.locked
            onClicked: Controls.restoreDefaults()
        }
    }
}
