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
                text: "OpenGP doesn't play GP DX. It is a different kind of game from GP 1 and GP 2, "
                      + "and other projects already run it on a PC."
                color: Theme.chalk
                font.pixelSize: Theme.lead + 1
                lineHeight: 1.25
                wrapMode: Text.WordWrap
            }

            Card {
                title: "Why not in OpenGP"
                body: "GP 1 and GP 2 run on Triforce, arcade hardware built from a GameCube; OpenGP is a port "
                      + "of those two games' own code to the PC. GP DX (2013) runs on Namco System ES3, which "
                      + "is a Windows PC (Windows Embedded Standard 7), so it is a PC program already. What it "
                      + "needs is the cabinet around it: the wheel and pedals, the card reader, the camera and "
                      + "the cabinet's I/O. That is a loader's job, not a port's."
            }

            Card {
                title: "On a Windows PC: TeknoParrot"
                tag: "Windows only"
                body: "TeknoParrot runs the game's own files and stands in for the cabinet. Its compatibility "
                      + "list has four versions of GP DX, all working perfectly: 1.10, 1.18, the US version "
                      + "and US 1.06.\n\n"
                      + "1. Install TeknoParrot, with the DirectX End-User Runtimes (June 2010) and the "
                      + "Visual C++ redistributables its download page asks for.\n"
                      + "2. In TeknoParrotUI, add the profile for your version of the game and point it at "
                      + "`MK_AGP3_FINAL.exe` in the game's folder.\n"
                      + "3. Set up the controls (a wheel, a gamepad or the keyboard), and the window size if "
                      + "you want something other than 1360×768.\n"
                      + "4. Mario Cards are Banapassport cards: signed in on teknoparrot.com, make a card ID "
                      + "in your online profile for the game and enter it in TeknoParrotUI. In the game, "
                      + "press **F2** to touch the card.\n"
                      + "5. If you want them: the webcam (the profile's webcam option) and force feedback "
                      + "(TeknoParrot's FFB Blaster, or FFBArcadePlugin).\n\n"
                      + "TeknoParrot is free; its paid plans add extras such as online play."
                links: [
                    { label: "TeknoParrot", url: "https://teknoparrot.com/en" },
                    { label: "GP DX on TeknoParrot", url: "https://teknoparrot.com/en/Compatibility/GameDetail/MKDX" },
                    { label: "FFBArcadePlugin", url: "https://github.com/Boomslangnz/FFBArcadePlugin" }
                ]
            }

            Card {
                title: "If it goes wrong"
                body: "- **A graphics error or a tiny window at the start**: DirectX files are missing. "
                      + "Install the DirectX End-User Runtimes (June 2010).\n"
                      + "- **The camera's picture is turned the wrong way**: the game can't fix this. Turn "
                      + "the camera itself, or use another camera.\n"
                      + "- **Anything else**: TeknoParrot's own page for the game lists what is known."
            }

            Card {
                title: "On a Mac or Linux"
                body: "TeknoParrot has no macOS version. Its Linux build is experimental, and we don't know "
                      + "of GP DX running on it, or under Wine or Proton. For now, GP DX needs a Windows PC "
                      + "or a cabinet."
            }

            Card {
                title: "The real thing"
                body: "A GP DX cabinet has everything the game expects: the wheel with force feedback, the "
                      + "card reader, the camera and linked play. Cabinets do come up second-hand, from "
                      + "arcades and their suppliers."
            }

            Card {
                title: "The game's files"
                body: "GP DX comes on the cabinet's hard drive. Neither OpenGP nor TeknoParrot includes it, "
                      + "and TeknoParrot won't link to it: use a copy of a drive you own. The launcher can't "
                      + "add GP DX, and this page doesn't download anything."
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
