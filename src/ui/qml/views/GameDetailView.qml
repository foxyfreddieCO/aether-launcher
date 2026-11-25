import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Item {
    id: detail

    property string gameId: ""
    property var game: ({})
    property bool editExpanded: false

    signal backRequested()
    signal launchRequested(string gameId)

    function refresh() {
        if (!gameId) return
        game = library.getGameById(gameId)
        if (!game || !game.id) {
            backRequested()
            return
        }
        titleField.text = game.title || ""
        platformField.text = game.platform || ""
        categoryField.text = game.category || ""
        developerField.text = game.developer || ""
        yearField.text = game.releaseYear || ""
        descField.text = game.description || ""
        exeField.text = game.executablePath || ""
        argsField.text = game.customArguments || ""
        workDirField.text = game.workingDirectory || ""
        syncEmulatorCombo()
        previewText.text = launcher.buildLaunchPreview(gameId)
    }

    function syncEmulatorCombo() {
        var idx = 0
        var emus = emulators.emulators
        for (var i = 0; i < emus.length; ++i) {
            if (emus[i].id === (game.emulatorId || "")) {
                idx = i + 1
                break
            }
        }
        emuCombo.currentIndex = idx
    }

    onGameIdChanged: refresh()

    Connections {
        target: library
        function onGamesChanged() {
            if (detail.visible && detail.gameId) detail.refresh()
        }
    }

    FileDialog {
        id: coverPicker
        title: "Choose Custom Cover Artwork"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: {
            library.setCustomCover(detail.gameId, selectedFile.toString())
            detail.refresh()
        }
    }

    FileDialog {
        id: bgPicker
        title: "Choose Custom Background Artwork"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: {
            library.setCustomBackground(detail.gameId, selectedFile.toString())
            detail.refresh()
        }
    }

    Flickable {
        anchors.fill: parent
        anchors.leftMargin: 42
        anchors.rightMargin: 42
        anchors.topMargin: 16
        anchors.bottomMargin: 24
        contentHeight: mainCol.implicitHeight + 40
        clip: true

        ColumnLayout {
            id: mainCol
            width: parent.width
            spacing: 22

            // Top Back & Quick Actions
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                FocusButton {
                    text: "← Back to Library"
                    onClicked: {
                        audio.playBack()
                        detail.backRequested()
                    }
                }
                Item { Layout.fillWidth: true }
                FocusButton {
                    text: editExpanded ? "Hide Game Settings" : "Game Settings & Launch Overrides"
                    selected: editExpanded
                    onClicked: editExpanded = !editExpanded
                }
            }

            // Main Hero Card
            GlassPanel {
                Layout.fillWidth: true
                implicitHeight: Math.max(380, heroRow.implicitHeight + 56)
                glassRadius: 28
                elevated: true
                opacityBoost: 0.12

                RowLayout {
                    id: heroRow
                    anchors.fill: parent
                    anchors.margins: 28
                    spacing: 32

                    // Large Cover Artwork
                    GlassPanel {
                        Layout.preferredWidth: 230
                        Layout.preferredHeight: 330
                        Layout.alignment: Qt.AlignTop
                        glassRadius: 20
                        clip: true

                        Rectangle {
                            anchors.fill: parent
                            visible: coverImage.status !== Image.Ready
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: Qt.darker(settings.accentColor, 1.4) }
                                GradientStop { position: 1.0; color: "#090D16" }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: (detail.game.title || "G").slice(0, 2).toUpperCase()
                                color: Qt.rgba(1, 1, 1, 0.28)
                                font.family: settings.fontFamily
                                font.pixelSize: 64
                                font.weight: Font.Black
                            }
                        }

                        Image {
                            id: coverImage
                            anchors.fill: parent
                            source: (detail.game.coverPath || "").length > 0 ? ("file://" + detail.game.coverPath) : ""
                            fillMode: Image.PreserveAspectCrop
                            smooth: true
                        }
                    }

                    // Title, Metadata, Play & Artwork Controls
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 14

                        Row {
                            spacing: 10
                            Rectangle {
                                height: 26
                                width: platText.implicitWidth + 20
                                radius: 13
                                color: Qt.rgba(settings.accentColor.r, settings.accentColor.g, settings.accentColor.b, 0.26)
                                border.width: 1
                                border.color: settings.accentColor
                                Text {
                                    id: platText
                                    anchors.centerIn: parent
                                    text: detail.game.platform || "Windows"
                                    color: "#FFFFFF"
                                    font.family: settings.fontFamily
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                            Rectangle {
                                visible: (detail.game.category || "").length > 0
                                height: 26
                                width: catText.implicitWidth + 18
                                radius: 13
                                color: Qt.rgba(1, 1, 1, 0.12)
                                Text {
                                    id: catText
                                    anchors.centerIn: parent
                                    text: detail.game.category || ""
                                    color: "#DCE8FA"
                                    font.family: settings.fontFamily
                                    font.pixelSize: 12
                                }
                            }
                            Text {
                                visible: (detail.game.developer || "").length > 0 || (detail.game.releaseYear || "").length > 0
                                text: [detail.game.developer || "", detail.game.releaseYear || ""].filter(function(s){ return s.length > 0 }).join("  •  ")
                                color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                                font.family: settings.fontFamily
                                font.pixelSize: 13
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: detail.game.title || ""
                            color: "#FFFFFF"
                            font.family: settings.fontFamily
                            font.pixelSize: 38
                            font.weight: Font.Bold
                            wrapMode: Text.WordWrap
                        }

                        Text {
                            Layout.fillWidth: true
                            text: (detail.game.description || "").length > 0
                                ? detail.game.description
                                : "No description set yet. Click Game Settings to add custom notes, description, developer, or release year."
                            color: Qt.rgba(0.88, 0.93, 1.0, 0.84)
                            font.family: settings.fontFamily
                            font.pixelSize: 15
                            wrapMode: Text.WordWrap
                        }

                        // Stats Row
                        Row {
                            spacing: 24
                            Column {
                                spacing: 2
                                Text { text: "PLAYTIME"; color: Qt.rgba(0.75, 0.85, 1, 0.6); font.pixelSize: 10; font.bold: true }
                                Text { text: detail.game.formattedPlaytime || "Never played"; color: "#FFFFFF"; font.pixelSize: 16; font.bold: true; font.family: settings.fontFamily }
                            }
                            Column {
                                spacing: 2
                                Text { text: "LAST PLAYED"; color: Qt.rgba(0.75, 0.85, 1, 0.6); font.pixelSize: 10; font.bold: true }
                                Text { text: detail.game.formattedLastPlayed || "Never"; color: "#FFFFFF"; font.pixelSize: 16; font.bold: true; font.family: settings.fontFamily }
                            }
                            Column {
                                spacing: 2
                                Text { text: "SESSIONS"; color: Qt.rgba(0.75, 0.85, 1, 0.6); font.pixelSize: 10; font.bold: true }
                                Text { text: String(detail.game.playCount || 0); color: "#FFFFFF"; font.pixelSize: 16; font.bold: true; font.family: settings.fontFamily }
                            }
                        }

                        // Primary Action Row
                        Flow {
                            Layout.fillWidth: true
                            spacing: 12

                            FocusButton {
                                text: "Play Game"
                                iconText: "▶"
                                primary: true
                                implicitWidth: 168
                                implicitHeight: 48
                                fontSize: 16
                                onClicked: detail.launchRequested(detail.gameId)
                            }
                            FocusButton {
                                text: detail.game.isFavorite ? "★ Favorited" : "☆ Add to Favorites"
                                implicitHeight: 48
                                onClicked: library.toggleFavorite(detail.gameId)
                            }
                            FocusButton {
                                text: "Set Cover Art"
                                implicitHeight: 48
                                onClicked: coverPicker.open()
                            }
                            FocusButton {
                                text: "Set Background"
                                implicitHeight: 48
                                onClicked: bgPicker.open()
                            }
                            FocusButton {
                                text: "Fetch Online Art"
                                implicitHeight: 48
                                onClicked: library.fetchOnlineArtwork(detail.gameId)
                            }
                        }

                        // Collections Toggle Strip
                        Column {
                            visible: library.collections.length > 0
                            width: parent.width
                            spacing: 6

                            Text {
                                text: "COLLECTIONS"
                                color: Qt.rgba(0.75, 0.85, 1, 0.65)
                                font.family: settings.fontFamily
                                font.pixelSize: 11
                                font.bold: true
                            }

                            Flow {
                                width: parent.width
                                spacing: 8
                                Repeater {
                                    model: library.collections
                                    FocusButton {
                                        readonly property bool inCol: library.isGameInCollection(modelData.id, detail.gameId)
                                        text: (inCol ? "✓ " : "+ ") + modelData.name
                                        selected: inCol
                                        implicitHeight: 32
                                        buttonRadius: 16
                                        fontSize: 12
                                        onClicked: library.setGameInCollection(modelData.id, detail.gameId, !inCol)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Per-Game Settings & Launch Override Panel
            GlassPanel {
                visible: detail.editExpanded
                Layout.fillWidth: true
                implicitHeight: editCol.implicitHeight + 48
                glassRadius: 24
                opacityBoost: 0.14

                ColumnLayout {
                    id: editCol
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 14

                    Text {
                        text: "Per-Game Metadata & Launch Settings"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 20
                        font.weight: Font.Bold
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Game Title"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: titleField
                                Layout.fillWidth: true
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Platform"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: platformField
                                Layout.fillWidth: true
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Category / Genre"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: categoryField
                                Layout.fillWidth: true
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Developer"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: developerField
                                Layout.fillWidth: true
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Release Year"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: yearField
                                Layout.fillWidth: true
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Assigned Emulator"; color: "#B9CBE5"; font.pixelSize: 12 }
                            ComboBox {
                                id: emuCombo
                                Layout.fillWidth: true
                                model: {
                                    var items = ["Direct Executable (No Emulator)"]
                                    var list = emulators.emulators
                                    for (var i = 0; i < list.length; ++i) {
                                        items.push(list[i].name + " (" + list[i].platform + ")")
                                    }
                                    return items
                                }
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                                contentItem: Text {
                                    leftPadding: 12
                                    text: emuCombo.displayText
                                    color: "#FFFFFF"
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            Text { text: "Game Executable / ROM Path"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: exeField
                                Layout.fillWidth: true
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Custom Launch Arguments Override (supports {rom}, {rom_dir}, {rom_name})"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: argsField
                                Layout.fillWidth: true
                                placeholderText: "Leave empty to use emulator default arguments"
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Working Directory Override (optional)"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextField {
                                id: workDirField
                                Layout.fillWidth: true
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            Text { text: "Description"; color: "#B9CBE5"; font.pixelSize: 12 }
                            TextArea {
                                id: descField
                                Layout.fillWidth: true
                                Layout.preferredHeight: 82
                                color: "#FFFFFF"
                                wrapMode: TextEdit.Wrap
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }
                    }

                    // Live Launch Command Preview
                    GlassPanel {
                        Layout.fillWidth: true
                        implicitHeight: previewText.implicitHeight + 22
                        glassRadius: 12
                        Text {
                            id: previewText
                            anchors.fill: parent
                            anchors.margins: 11
                            color: "#9FD8FF"
                            font.family: "Consolas, Menlo, monospace"
                            font.pixelSize: 12
                            wrapMode: Text.WrapAnywhere
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        FocusButton {
                            text: "Save Changes"
                            primary: true
                            onClicked: {
                                var emuId = ""
                                if (emuCombo.currentIndex > 0) {
                                    var list = emulators.emulators
                                    if (emuCombo.currentIndex - 1 < list.length) {
                                        emuId = list[emuCombo.currentIndex - 1].id
                                    }
                                }
                                library.updateGame(detail.gameId, {
                                    title: titleField.text,
                                    platform: platformField.text,
                                    category: categoryField.text,
                                    developer: developerField.text,
                                    releaseYear: yearField.text,
                                    description: descField.text,
                                    executablePath: exeField.text,
                                    customArguments: argsField.text,
                                    workingDirectory: workDirField.text,
                                    emulatorId: emuId
                                })
                                detail.refresh()
                            }
                        }

                        Item { Layout.fillWidth: true }

                        FocusButton {
                            text: "Remove from Library"
                            danger: true
                            onClicked: {
                                library.removeGame(detail.gameId)
                                detail.backRequested()
                            }
                        }
                    }
                }
            }
        }
    }
}
