import QtQuick

Item {
    id: card

    property var gameData: ({})
    property bool isCurrent: false
    property bool compactList: false

    signal activated()
    signal selected()
    signal detailsRequested()

    readonly property bool hasCover: (gameData.coverPath || "").length > 0
    readonly property string coverUrl: hasCover ? ("file://" + gameData.coverPath) : ""
    readonly property string titleText: gameData.title || "Untitled"
    readonly property string platformText: gameData.platform || "Windows"
    readonly property string playtimeText: gameData.formattedPlaytime || ""
    readonly property bool isFav: !!gameData.isFavorite

    scale: isCurrent ? (compactList ? 1.01 : 1.06) : (mouseArea.containsMouse ? 1.02 : 0.96)
    z: isCurrent ? 10 : 1

    Behavior on scale {
        enabled: settings.animationsEnabled
        NumberAnimation { duration: Math.round(200 / settings.animationSpeed); easing.type: Easing.OutBack; easing.overshoot: 0.7 }
    }

    function platformTint() {
        var p = platformText.toLowerCase()
        if (p.indexOf("switch") >= 0) return "#E60039"
        if (p.indexOf("playstation") >= 0 || p.indexOf("ps") >= 0) return "#2E6BFF"
        if (p.indexOf("xbox") >= 0) return "#10B981"
        if (p.indexOf("gamecube") >= 0 || p.indexOf("wii") >= 0) return "#8B5CF6"
        return settings.accentColor
    }

    function monogram() {
        var words = titleText.trim().split(/\s+/)
        if (words.length >= 2) return (words[0][0] + words[1][0]).toUpperCase()
        return titleText.slice(0, 2).toUpperCase()
    }

    GlassPanel {
        anchors.fill: parent
        glassRadius: compactList ? 16 : 22
        focused: card.isCurrent || mouseArea.containsMouse
        elevated: card.isCurrent
        accentColor: settings.accentColor
        opacityBoost: card.isCurrent ? 0.14 : 0.02

        Item {
            anchors.fill: parent
            anchors.margins: 3
            clip: true

            // Standard Ribbon / Grid Cover Layout
            Item {
                anchors.fill: parent
                visible: !card.compactList

                // Procedural glass cover fallback
                Rectangle {
                    anchors.fill: parent
                    radius: 19
                    visible: coverImg.status !== Image.Ready
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.darker(card.platformTint(), 1.45) }
                        GradientStop { position: 0.55; color: Qt.rgba(0.08, 0.11, 0.18, 0.95) }
                        GradientStop { position: 1.0; color: Qt.rgba(0.04, 0.05, 0.09, 0.98) }
                    }

                    Text {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -22
                        text: card.monogram()
                        color: Qt.rgba(1, 1, 1, 0.24)
                        font.family: settings.fontFamily
                        font.pixelSize: Math.min(parent.width * 0.34, 72)
                        font.weight: Font.Black
                    }
                }

                // Real cover artwork
                Image {
                    id: coverImg
                    anchors.fill: parent
                    source: card.coverUrl
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    smooth: true
                    opacity: status === Image.Ready ? 1.0 : 0.0
                    Behavior on opacity {
                        enabled: settings.animationsEnabled
                        NumberAnimation { duration: 220 }
                    }
                }

                // Bottom readable gradient
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: parent.height * 0.48
                    radius: 19
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.55; color: Qt.rgba(0.02, 0.04, 0.08, 0.78) }
                        GradientStop { position: 1.0; color: Qt.rgba(0.02, 0.03, 0.06, 0.96) }
                    }
                }

                // Top badges (platform + favorite)
                Row {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 12
                    spacing: 6

                    Rectangle {
                        height: 22
                        width: platLbl.implicitWidth + 16
                        radius: 11
                        color: Qt.rgba(0.05, 0.07, 0.12, 0.78)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.22)

                        Text {
                            id: platLbl
                            anchors.centerIn: parent
                            text: card.platformText
                            color: "#EAF3FF"
                            font.family: settings.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                        }
                    }

                    Item { width: 1; height: 1 }

                    Rectangle {
                        visible: card.isFav
                        width: 22
                        height: 22
                        radius: 11
                        color: Qt.rgba(1.0, 0.78, 0.18, 0.90)
                        Text {
                            anchors.centerIn: parent
                            text: "★"
                            color: "#090D16"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }

                // Title + Playtime footer
                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 3

                    Text {
                        width: parent.width
                        text: card.titleText
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: card.playtimeText
                        color: Qt.rgba(0.82, 0.89, 1.0, 0.78)
                        font.family: settings.fontFamily
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }

            // Compact List Layout
            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 14
                visible: card.compactList

                Rectangle {
                    width: 48
                    height: parent.height
                    radius: 10
                    clip: true
                    color: Qt.darker(card.platformTint(), 1.5)

                    Image {
                        anchors.fill: parent
                        source: card.coverUrl
                        fillMode: Image.PreserveAspectCrop
                        visible: status === Image.Ready
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: !card.hasCover
                        text: card.monogram()
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 16
                        font.bold: true
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 210
                    spacing: 2

                    Text {
                        width: parent.width
                        text: (card.isFav ? "★ " : "") + card.titleText
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        text: card.platformText + "  •  " + (card.gameData.category || "Game")
                        color: Qt.rgba(0.80, 0.87, 0.98, 0.75)
                        font.family: settings.fontFamily
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 130
                    horizontalAlignment: Text.AlignRight
                    text: card.playtimeText
                    color: Qt.rgba(0.85, 0.92, 1.0, 0.82)
                    font.family: settings.fontFamily
                    font.pixelSize: 12
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: {
            controller.notifyKeyboardMouseActivity()
            card.selected()
        }
        onClicked: {
            controller.notifyKeyboardMouseActivity()
            card.selected()
            audio.playConfirm()
            card.detailsRequested()
        }
        onDoubleClicked: {
            controller.notifyKeyboardMouseActivity()
            card.activated()
        }
    }
}
