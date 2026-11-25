import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    id: root
    anchors.fill: parent
    z: 200

    property bool errorVisible: false
    property string errorGameTitle: ""
    property string errorMessage: ""

    signal openEmulatorsRequested()

    function showError(title, reason) {
        errorGameTitle = title
        errorMessage = reason
        errorVisible = true
    }

    visible: launcher.isRunning || errorVisible

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.04, 0.08, 0.84)
    }

    // Active Game Running Card
    GlassPanel {
        visible: launcher.isRunning && !root.errorVisible
        anchors.centerIn: parent
        width: Math.min(parent.width - 60, 560)
        implicitHeight: runningCol.implicitHeight + 60
        glassRadius: 28
        elevated: true
        opacityBoost: 0.22

        ColumnLayout {
            id: runningCol
            anchors.fill: parent
            anchors.margins: 30
            spacing: 14

            Text {
                text: "NOW PLAYING"
                color: settings.accentColor
                font.family: settings.fontFamily
                font.pixelSize: 12
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: launcher.activeGameTitle
                color: "#FFFFFF"
                font.family: settings.fontFamily
                font.pixelSize: 28
                font.weight: Font.Bold
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                text: "Session Time: " + library.formatPlaytime(launcher.activeSessionSeconds)
                color: "#D8E8FF"
                font.family: settings.fontFamily
                font.pixelSize: 16
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "Aether Launcher will return automatically and save your playtime when the game closes."
                color: Qt.rgba(0.84, 0.90, 1, 0.72)
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            FocusButton {
                text: "Force Close Game"
                danger: true
                Layout.alignment: Qt.AlignHCenter
                onClicked: launcher.forceStopActiveGame()
            }
        }
    }

    // Graceful Launch Failure Modal
    GlassPanel {
        visible: root.errorVisible
        anchors.centerIn: parent
        width: Math.min(parent.width - 60, 580)
        implicitHeight: errCol.implicitHeight + 56
        glassRadius: 28
        elevated: true
        accentColor: "#FF5C7A"
        opacityBoost: 0.26

        ColumnLayout {
            id: errCol
            anchors.fill: parent
            anchors.margins: 28
            spacing: 14

            Text {
                text: "Unable to Launch \"" + root.errorGameTitle + "\""
                color: "#FF8DA1"
                font.family: settings.fontFamily
                font.pixelSize: 22
                font.bold: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                text: root.errorMessage
                color: "#EEF4FF"
                font.family: settings.fontFamily
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                FocusButton {
                    text: "Configure Emulators"
                    primary: true
                    onClicked: {
                        root.errorVisible = false
                        root.openEmulatorsRequested()
                    }
                }
                Item { Layout.fillWidth: true }
                FocusButton {
                    text: "Dismiss"
                    onClicked: {
                        audio.playBack()
                        root.errorVisible = false
                    }
                }
            }
        }
    }
}
