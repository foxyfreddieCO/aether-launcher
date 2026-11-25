import QtQuick
import QtQuick.Layouts
import "../components"

Flickable {
    id: widgetFlick
    contentWidth: row.implicitWidth
    contentHeight: row.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentWidth > width

    signal openQuickMenu()
    signal openRecentGame(string gameId)

    Row {
        id: row
        spacing: 10
        height: widgetFlick.height

        Repeater {
            model: settings.widgets

            Loader {
                active: !!modelData.enabled
                visible: active
                height: widgetFlick.height
                property string wSize: modelData.size || "medium"
                property string wId: modelData.id || ""

                sourceComponent: {
                    switch (wId) {
                    case "clock": return clockWidget
                    case "telemetry": return telemetryWidget
                    case "audio": return audioWidget
                    case "controller": return controllerWidget
                    case "wifi": return wifiWidget
                    case "bluetooth": return bluetoothWidget
                    case "recent": return recentWidget
                    case "stats": return statsWidget
                    default: return null
                    }
                }
            }
        }
    }

    Component {
        id: clockWidget
        GlassPanel {
            glassRadius: 16
            implicitWidth: wSize === "compact" ? 105 : (wSize === "large" ? 205 : 160)
            height: parent.height

            Row {
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: sys.currentTime
                    color: "#FFFFFF"
                    font.family: settings.fontFamily
                    font.pixelSize: wSize === "large" ? 18 : 15
                    font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    visible: wSize !== "compact"
                    text: sys.currentDate
                    color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Component {
        id: telemetryWidget
        GlassPanel {
            glassRadius: 16
            implicitWidth: teleRow.implicitWidth + 24
            height: parent.height

            Row {
                id: teleRow
                anchors.centerIn: parent
                spacing: 12

                Text {
                    text: "CPU " + Math.round(sys.cpuUsage) + "%"
                    color: "#E8F1FF"
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Text {
                    visible: sys.gpuAvailable && wSize !== "compact"
                    text: "GPU " + Math.round(sys.gpuUsage) + "%"
                    color: settings.accentColor
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Text {
                    text: wSize === "large"
                        ? ("RAM " + sys.ramUsedGb.toFixed(1) + "/" + sys.ramTotalGb.toFixed(0) + "G")
                        : ("RAM " + Math.round(sys.ramUsage) + "%")
                    color: "#D3E2F7"
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
                Text {
                    visible: sys.temperatureAvailable && wSize === "large"
                    text: Math.round(sys.temperatureC) + "°C"
                    color: "#FFB86C"
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                }
            }
        }
    }

    Component {
        id: audioWidget
        GlassPanel {
            glassRadius: 16
            implicitWidth: audioRow.implicitWidth + 22
            height: parent.height

            Row {
                id: audioRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: sys.systemMuted ? "🔇" : "🔊"
                    font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: sys.systemMuted ? "Muted" : (sys.systemVolume + "%")
                    color: "#FFFFFF"
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    visible: wSize === "large"
                    text: "• " + sys.currentAudioDeviceName
                    color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                    font.family: settings.fontFamily
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    width: 110
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    audio.playConfirm()
                    widgetFlick.openQuickMenu()
                }
            }
        }
    }

    Component {
        id: controllerWidget
        GlassPanel {
            glassRadius: 16
            implicitWidth: ctrlRow.implicitWidth + 22
            height: parent.height

            Row {
                id: ctrlRow
                anchors.centerIn: parent
                spacing: 7

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: controller.connected ? "#38F29A" : "#64748B"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: {
                        if (!controller.connected) return wSize === "compact" ? "KBM" : "Keyboard & Mouse"
                        if (wSize === "compact") return controller.batteryPercent >= 0 ? (controller.batteryPercent + "%") : "Pad"
                        var bat = controller.batteryPercent >= 0 ? (" • " + controller.batteryPercent + "%") : (controller.batteryState === "wired" ? " • USB" : "")
                        return (wSize === "large" ? controller.primaryName : "Controller") + bat
                    }
                    color: "#EEF5FF"
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Component {
        id: wifiWidget
        GlassPanel {
            glassRadius: 16
            implicitWidth: wSize === "compact" ? 56 : 86
            height: parent.height

            Row {
                anchors.centerIn: parent
                spacing: 6
                Text { text: "Wi-Fi"; color: "#E9F2FF"; font.family: settings.fontFamily; font.pixelSize: 12; font.weight: Font.DemiBold }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    audio.playConfirm()
                    sys.openWifiSettings()
                }
            }
        }
    }

    Component {
        id: bluetoothWidget
        GlassPanel {
            glassRadius: 16
            implicitWidth: wSize === "compact" ? 44 : 102
            height: parent.height

            Row {
                anchors.centerIn: parent
                spacing: 5
                Text { text: "BT"; visible: wSize === "compact"; color: settings.accentColor; font.family: settings.fontFamily; font.pixelSize: 12; font.bold: true }
                Text { text: "Bluetooth"; visible: wSize !== "compact"; color: "#E9F2FF"; font.family: settings.fontFamily; font.pixelSize: 12; font.weight: Font.DemiBold }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    audio.playConfirm()
                    sys.openBluetoothSettings()
                }
            }
        }
    }

    Component {
        id: recentWidget
        GlassPanel {
            glassRadius: 16
            visible: library.recentGames.length > 0
            implicitWidth: visible ? Math.min(230, recRow.implicitWidth + 24) : 0
            height: parent.height

            Row {
                id: recRow
                anchors.centerIn: parent
                spacing: 6
                Text { text: "Recent:"; color: settings.accentColor; font.family: settings.fontFamily; font.pixelSize: 11; font.bold: true }
                Text {
                    text: library.recentGames.length > 0 ? library.recentGames[0].title : ""
                    color: "#FFFFFF"
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    width: Math.min(130, implicitWidth)
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (library.recentGames.length > 0) {
                        audio.playConfirm()
                        widgetFlick.openRecentGame(library.recentGames[0].id)
                    }
                }
            }
        }
    }

    Component {
        id: statsWidget
        GlassPanel {
            glassRadius: 16
            implicitWidth: statsRow.implicitWidth + 24
            height: parent.height

            Row {
                id: statsRow
                anchors.centerIn: parent
                spacing: 10
                Text {
                    text: library.totalGamesCount + " Games"
                    color: "#E8F1FF"
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Text {
                    visible: wSize !== "compact"
                    text: "• " + library.formatPlaytime(library.totalPlaytimeSeconds)
                    color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                }
            }
        }
    }
}
