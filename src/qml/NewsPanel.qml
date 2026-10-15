import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Announcements from the OpenGP team (the news feed), newest first; with no
// news, what changed in this version.
Rectangle {
    id: panel
    radius: 14
    color: Qt.rgba(0.082, 0.102, 0.145, 0.78)
    border.color: Theme.line

    readonly property bool hasNews: News.enabled && News.items.length > 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: panel.hasNews || !News.whatsNew ? "News" : News.whatsNewTitle
                color: Theme.chalk
                font.pixelSize: Theme.title
                font.weight: Font.ExtraBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                visible: News.offline && panel.hasNews
                text: "Offline"
                color: Theme.faint
                font.pixelSize: Theme.small
            }
            PillButton {
                visible: panel.hasNews && News.unread > 0
                text: News.unread === 1 ? "Mark 1 as read" : "Mark " + News.unread + " as read"
                onClicked: News.markAllSeen()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            visible: panel.hasNews

            ColumnLayout {
                width: parent.width
                spacing: 0
                Repeater {
                    model: News.items
                    delegate: ColumnLayout {
                        id: item
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        spacing: 6

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: item.index ? 14 : 0
                            Layout.bottomMargin: 8
                            height: 1
                            color: Theme.line
                            visible: item.index > 0
                        }
                        RowLayout {
                            spacing: 10
                            Text {
                                text: item.modelData.date
                                color: Theme.dim
                                font.pixelSize: Theme.small
                            }
                            Text {
                                text: item.modelData.tag === "update" ? "Update"
                                    : item.modelData.tag === "event" ? "Event" : "Announcement"
                                color: item.modelData.tag === "update" ? Theme.blue
                                     : item.modelData.tag === "event" ? Theme.marquee : Theme.dim
                                font.pixelSize: Theme.small
                                font.weight: Font.ExtraBold
                            }
                            Text {
                                visible: item.modelData.pinned
                                text: "Pinned"
                                color: Theme.faint
                                font.pixelSize: Theme.small
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: Theme.marquee
                                visible: item.modelData.unread
                            }
                            Text {
                                Layout.fillWidth: true
                                text: item.modelData.title
                                color: Theme.chalk
                                font.pixelSize: Theme.lead + 1
                                font.weight: Font.ExtraBold
                                wrapMode: Text.WordWrap
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: item.modelData.body.length > 0
                            text: item.modelData.body
                            textFormat: Text.MarkdownText
                            wrapMode: Text.WordWrap
                            color: "#C9D0DC"
                            linkColor: Theme.blue
                            font.pixelSize: Theme.body
                            lineHeight: 1.25
                            onLinkActivated: link => Launcher.openLink(link)
                            HoverHandler { cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor }
                        }
                        Text {
                            visible: item.modelData.link.length > 0
                            text: "Read more"
                            color: Theme.blue
                            font.pixelSize: Theme.body
                            font.underline: linkArea.containsMouse
                            MouseArea {
                                id: linkArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Launcher.openLink(item.modelData.link)
                            }
                        }
                    }
                }
            }
        }

        // No news: this version's changes.
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            visible: !panel.hasNews && News.whatsNew.length > 0
            Text {
                width: parent.width
                text: News.whatsNew
                textFormat: Text.MarkdownText
                wrapMode: Text.WordWrap
                color: "#C9D0DC"
                linkColor: Theme.blue
                font.pixelSize: Theme.body
                lineHeight: 1.25
                onLinkActivated: link => Launcher.openLink(link)
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !panel.hasNews && News.whatsNew.length === 0
            text: News.enabled ? "No news yet." : "News is off. Turn it on in Settings, under Launcher."
            color: Theme.dim
            font.pixelSize: Theme.body
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignTop
        }
    }
}
