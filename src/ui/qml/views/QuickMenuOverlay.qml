import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: overlay

    property bool isOpen: false
    property int focusedAction: 0

    signal closeRequested()
    signal navigateTo(string page)
    signal displayModeChanged()
    signal quitRequested()

    visible: isOpen || drawer.x < width - 2
    z: 100

    // Dimmed click-outside scrim
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.01, 0.02, 0.05, overlay.isOpen ? 0.58 : 0.0)
        Behavior on color {
            enabled: settings.animationsEnabled
            ColorAnimation { duration: Math.round(220 / settings.animationSpeed) }
        }
        MouseArea {
            anchors.fill: parent
            enabled: overlay.isOpen
            onClicked: {
                audio.playBack()
                overlay.closeRequested()
            }
        }
    }

    GlassPanel {
        id: drawer
        width: Math.min(430, overlay.width * 0.85)
        height: overlay.height - 32
        y: 16
        x: overlay.isOpen ? (overlay.width - width - 16) : (overlay.width + 20)
        glassRadius: 28
        elevated: true
        opacityBoost: 0.24

        Behavior on x {
            enabled: settings.animationsEnabled
            NumberAnimation { duration: Math.round(250 / settings.animationSpeed); easing.type: Easing.OutCubic }
        }

        Flickable {
            anchors.fill: parent
            anchors.margins: 24
            contentHeight: contentCol.implicitHeight + 20
            clip: true

            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: 16

                // Header Clock + Close
                RowLayout {
                    Layout.fillWidth: true
                    Column {
                        spacing: 2
                        Text { text: "Quick Settings"; color: "#FFFFFF"; font.family: settings.fontFamily; font.pixelSize: 22; font.bold: true }
                        Text { text: sys.currentTime + "  •  " + sys.currentDate; color: Qt.rgba(0.84, 0.90, 1, 0.72); font.pixelSize: 12 }
                    }
                    Item { Layout.fillWidth: true }
                    FocusButton {
                        text: "✕"
                        implicitWidth: 40
                        implicitHeight: 40
                        buttonRadius: 20
                        onClicked: {
                            audio.playBack()
                            overlay.closeRequested()
                        }
                    }
                }

                // System Volume + Audio Output Selection
                GlassPanel {
                    Layout.fillWidth: true
                    implicitHeight: volCol.implicitHeight + 28
                    glassRadius: 18
                    opacityBoost: 0.08

                    ColumnLayout {
                        id: volCol
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "System Volume"; color: "#E6F0FF"; font.pixelSize: 13; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Text { text: sys.systemMuted ? "MUTED" : (sys.systemVolume + "%"); color: settings.accentColor; font.pixelSize: 13; font.bold: true }
                            FocusButton {
                                text: sys.systemMuted ? "Unmute" : "Mute"
                                implicitWidth: 74
                                implicitHeight: 28
                                fontSize: 11
                                onClicked: sys.systemMuted = !sys.systemMuted
                            }
                        }

                        Slider {
                            Layout.fillWidth: true
                            from: 0; to: 100; stepSize: 2
                            value: sys.systemVolume
                            onMoved: sys.systemVolume = Math.round(value)
                        }

                        Text { text: "Audio Output Device"; color: "#A9C0E0"; font.pixelSize: 11; font.bold: true }

                        Repeater {
                            model: sys.audioOutputDevices
                            FocusButton {
                                Layout.fillWidth: true
                                text: (modelData.isDefault ? "● " : "○ ") + modelData.name
                                selected: !!modelData.isDefault
                                implicitHeight: 34
                                fontSize: 12
                                onClicked: sys.selectAudioOutputDevice(modelData.id)
                            }
                        }
                    }
                }

                // Hardware Telemetry Grid
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 10

                    GlassPanel {
                        Layout.fillWidth: true
                        implicitHeight: 62
                        glassRadius: 14
                        Column {
                            anchors.centerIn: parent
                            Text { text: "CPU"; color: "#9CB4D8"; font.pixelSize: 10; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: Math.round(sys.cpuUsage) + "%"; color: "#FFFFFF"; font.pixelSize: 18; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }

                    GlassPanel {
                        Layout.fillWidth: true
                        implicitHeight: 62
                        glassRadius: 14
                        Column {
                            anchors.centerIn: parent
                            Text { text: "RAM"; color: "#9CB4D8"; font.pixelSize: 10; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: sys.ramUsedGb.toFixed(1) + " / " + sys.ramTotalGb.toFixed(0) + " GB"; color: "#FFFFFF"; font.pixelSize: 15; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }

                    GlassPanel {
                        Layout.fillWidth: true
                        implicitHeight: 62
                        glassRadius: 14
                        Column {
                            anchors.centerIn: parent
                            Text { text: "GPU"; color: "#9CB4D8"; font.pixelSize: 10; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: sys.gpuAvailable ? (Math.round(sys.gpuUsage) + "%") : "N/A"; color: settings.accentColor; font.pixelSize: 16; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }

                    GlassPanel {
                        Layout.fillWidth: true
                        implicitHeight: 62
                        glassRadius: 14
                        Column {
                            anchors.centerIn: parent
                            Text { text: "CONTROLLER"; color: "#9CB4D8"; font.pixelSize: 10; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                            Text {
                                text: controller.connected
                                    ? (controller.batteryPercent >= 0 ? (controller.batteryPercent + "%") : "Connected")
                                    : "None"
                                color: controller.connected ? "#38F29A" : "#94A3B8"
                                font.pixelSize: 15
                                font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }

                // Connectivity & Quick Toggles
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 10

                    FocusButton {
                        Layout.fillWidth: true
                        text: "Bluetooth Settings"
                        implicitHeight: 40
                        fontSize: 12
                        onClicked: sys.openBluetoothSettings()
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: "Wi-Fi Settings"
                        implicitHeight: 40
                        fontSize: 12
                        onClicked: sys.openWifiSettings()
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: settings.performanceOverlay ? "Perf HUD: ON" : "Perf HUD: OFF"
                        selected: settings.performanceOverlay
                        implicitHeight: 40
                        fontSize: 12
                        onClicked: settings.performanceOverlay = !settings.performanceOverlay
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: settings.bgmEnabled ? "Ambient Music: ON" : "Ambient Music: OFF"
                        selected: settings.bgmEnabled
                        implicitHeight: 40
                        fontSize: 12
                        onClicked: settings.bgmEnabled = !settings.bgmEnabled
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: settings.fullscreen ? "Windowed Mode" : "Fullscreen Mode"
                        implicitHeight: 40
                        fontSize: 12
                        onClicked: {
                            settings.fullscreen = !settings.fullscreen
                            overlay.displayModeChanged()
                        }
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: settings.consoleMode ? "Desktop Mode" : "Console Mode"
                        implicitHeight: 40
                        fontSize: 12
                        onClicked: settings.consoleMode = !settings.consoleMode
                    }
                }

                // Quick Navigation Links
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text { text: "NAVIGATION"; color: "#8BA5C8"; font.pixelSize: 11; font.bold: true }

                    FocusButton {
                        Layout.fillWidth: true
                        text: "Home Dashboard"
                        onClicked: { overlay.closeRequested(); overlay.navigateTo("home") }
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: "Emulators & Lichii (Switch 2)"
                        onClicked: { overlay.closeRequested(); overlay.navigateTo("emulators") }
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: "Library, Collections & Backups"
                        onClicked: { overlay.closeRequested(); overlay.navigateTo("library") }
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: "Settings & Customization"
                        onClicked: { overlay.closeRequested(); overlay.navigateTo("settings") }
                    }
                    FocusButton {
                        Layout.fillWidth: true
                        text: "Exit Aether Launcher"
                        danger: true
                        onClicked: overlay.quitRequested()
                    }
                }
            }
        }
    }
}
