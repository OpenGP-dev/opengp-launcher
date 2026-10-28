import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Mario Kart Arcade GP DX: not OpenGP's (it is not a Triforce game), so why,
// and how people play it otherwise. Nothing here downloads or adds anything.
Item {
    id: dx

    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.topMargin: 36
        anchors.bottomMargin: 24
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            x: 48
            width: Math.min(scroll.availableWidth - 96, 860)
            spacing: 20

            Text {
                Layout.fillWidth: true
                text: "Mario Kart Arcade GP DX"
                color: Theme.chalk
                font.pixelSize: Theme.display + 6
                font.weight: Font.ExtraBold
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                text: "OpenGP doesn't play GP DX. GP 1 and GP 2 are Triforce games; GP DX runs on a "
                      + "Windows PC (Namco System ES3), so it needs a loader, not a port."
                color: Theme.chalk
                font.pixelSize: Theme.lead + 1
                lineHeight: 1.25
                wrapMode: Text.WordWrap
            }

            Card {
                title: "Play it with TeknoParrot"
                tag: "Windows only"
                body: "1. Install TeknoParrot, with the DirectX (June 2010) and Visual C++ runtimes.\n"
                      + "2. Add the GP DX profile for your version and choose `MK_AGP3_FINAL.exe`.\n"
                      + "3. Set up your controls.\n"
                      + "4. For Mario Cards, make a card ID in your TeknoParrot online profile. "
                      + "Press **F2** in the game to use it."
                links: [
                    { label: "TeknoParrot", url: "https://teknoparrot.com/en" },
                    { label: "GP DX on TeknoParrot", url: "https://teknoparrot.com/en/Compatibility/GameDetail/MKDX" },
                    { label: "FFBArcadePlugin", url: "https://github.com/Boomslangnz/FFBArcadePlugin" }
                ]
            }

            Card {
                title: "If it goes wrong"
                body: "- **Graphics error or a tiny window**: install the DirectX runtimes.\n"
                      + "- **Camera picture turned**: turn the camera itself."
            }

            Card {
                title: "Mac, Linux or a cabinet"
                body: "TeknoParrot has no Mac version, and its Linux build is untested with GP DX. "
                      + "Otherwise, GP DX cabinets do come up second-hand."
            }

            Text {
                Layout.fillWidth: true
                text: "Use a copy of a cabinet drive you own: neither OpenGP nor TeknoParrot includes the game."
                color: Theme.dim
                font.pixelSize: Theme.small
                wrapMode: Text.WordWrap
            }

            Item { implicitHeight: 4 }
        }
    }

    // A heading, Markdown and the links that go with it.
    component Card: Rectangle {
        id: card
        property string title
        property string tag
        property string body
        property var links: []

        Layout.fillWidth: true
        implicitHeight: content.implicitHeight + 44
        radius: 14
        color: Qt.rgba(0.082, 0.102, 0.145, 0.78)
        border.color: Theme.line

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 22
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    Layout.fillWidth: true
                    text: card.title
                    color: Theme.chalk
                    font.pixelSize: Theme.title
                    font.weight: Font.ExtraBold
                    wrapMode: Text.WordWrap
                }
                Text {
                    visible: card.tag.length > 0
                    text: card.tag
                    color: Theme.marquee
                    font.pixelSize: Theme.small
                    font.weight: Font.ExtraBold
                }
            }
            Text {
                Layout.fillWidth: true
                text: card.body
                textFormat: Text.MarkdownText
                wrapMode: Text.WordWrap
                color: "#C9D0DC"
                linkColor: Theme.blue
                font.pixelSize: Theme.body
                lineHeight: 1.25
                onLinkActivated: link => Launcher.openLink(link)
            }
            Flow {
                Layout.fillWidth: true
                Layout.topMargin: 4
                visible: card.links.length > 0
                spacing: 10
                Repeater {
                    model: card.links
                    delegate: PillButton {
                        required property var modelData
                        text: modelData.label
                        onClicked: Launcher.openLink(modelData.url)
                    }
                }
            }
        }
    }
}
