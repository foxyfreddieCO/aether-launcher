import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: home

    property int selectedIndex: 0
    readonly property var gamesModel: library.filteredGames
    readonly property var currentGame: (gamesModel.length > 0 && selectedIndex >= 0 && selectedIndex < gamesModel.length)
        ? gamesModel[selectedIndex]
        : null

    signal openGameDetails(string gameId)
    signal launchGameRequested(string gameId)
    signal openEmulatorsRequested()
    signal openLibraryManagerRequested()
    signal addExecutableRequested()
    signal addRomRequested()
    signal scanFolderRequested()

    onGamesModelChanged: {
        if (gamesModel.length === 0) {
            selectedIndex = 0
        } else if (selectedIndex >= gamesModel.length) {
            selectedIndex = gamesModel.length - 1
        }
    }

    function moveSelection(delta) {
        if (gamesModel.length === 0) return
        var next = Math.max(0, Math.min(gamesModel.length - 1, selectedIndex + delta))
        if (next !== selectedIndex) {
            selectedIndex = next
            audio.playNavigate()
            if (settings.libraryLayout === "ribbon") {
                ribbonView.positionViewAtIndex(selectedIndex, ListView.Contain)
            } else if (settings.libraryLayout === "grid") {
                gridView.positionViewAtIndex(selectedIndex, GridView.Contain)
            } else {
                listView.positionViewAtIndex(selectedIndex, ListView.Contain)
            }
        }
    }

    function handleNavigate(direction) {
        if (gamesModel.length === 0) return
        if (settings.libraryLayout === "grid") {
            var cols = Math.max(1, Math.floor(gridView.width / gridView.cellWidth))
            if (direction === "left") moveSelection(-1)
            else if (direction === "right") moveSelection(1)
            else if (direction === "up") moveSelection(-cols)
            else if (direction === "down") moveSelection(cols)
        } else if (settings.libraryLayout === "list") {
            if (direction === "up") moveSelection(-1)
            else if (direction === "down") moveSelection(1)
        } else {
            if (direction === "left") moveSelection(-1)
            else if (direction === "right") moveSelection(1)
        }
    }

    function activateCurrent() {
        if (!currentGame) return
        audio.playConfirm()
        openGameDetails(currentGame.id)
    }

    function toggleCurrentFavorite() {
        if (!currentGame) return
        audio.playConfirm()
        library.toggleFavorite(currentGame.id)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 36
        anchors.rightMargin: 36
        anchors.topMargin: 12
        anchors.bottomMargin: 18
        spacing: 16

        // Category & Collection Filter Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Section pills (LB/RB)
            Row {
                spacing: 8
                Repeater {
                    model: [
                        { id: "all", label: "All Games" },
                        { id: "favorites", label: "Favorites" },
                        { id: "recent", label: "Recently Played" },
                        { id: "added", label: "Recently Added" },
                        { id: "most_played", label: "Most Played" }
                    ]

                    FocusButton {
                        text: modelData.label
                        selected: library.activeSection === modelData.id
                        implicitHeight: 36
                        buttonRadius: 18
                        fontSize: 13
                        onClicked: library.activeSection = modelData.id
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Platform filter selector
            ComboBox {
                id: platformBox
                visible: library.availablePlatforms.length > 0
                implicitWidth: 175
                implicitHeight: 36
                model: ["All Platforms"].concat(library.availablePlatforms)
                onActivated: {
                    library.platformFilter = currentIndex === 0 ? "" : currentText
                }
                background: GlassPanel {
                    glassRadius: 14
                    opacityBoost: 0.08
                }
                contentItem: Text {
                    leftPadding: 12
                    rightPadding: 24
                    text: platformBox.displayText
                    color: "#EFF6FF"
                    font.family: settings.fontFamily
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            // Search box
            GlassPanel {
                Layout.preferredWidth: 230
                Layout.preferredHeight: 36
                glassRadius: 18
                focused: searchInput.activeFocus
                opacityBoost: 0.08

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 10
                    spacing: 6

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 13
                        clip: true
                        text: library.searchQuery
                        onTextChanged: library.searchQuery = text

                        Text {
                            visible: searchInput.text.length === 0
                            text: "Search library..."
                            color: Qt.rgba(0.80, 0.87, 0.98, 0.55)
                            font.family: settings.fontFamily
                            font.pixelSize: 13
                        }
                    }

                    Text {
                        visible: searchInput.text.length > 0
                        text: "✕"
                        color: "#B8C7DC"
                        font.pixelSize: 12
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                searchInput.text = ""
                                library.searchQuery = ""
                            }
                        }
                    }
                }
            }

            // Layout toggle pill (Ribbon / Grid / List)
            FocusButton {
                text: settings.libraryLayout === "ribbon" ? "Ribbon" : (settings.libraryLayout === "grid" ? "Grid" : "List")
                implicitHeight: 36
                implicitWidth: 88
                buttonRadius: 14
                fontSize: 12
                onClicked: {
                    if (settings.libraryLayout === "ribbon") settings.libraryLayout = "grid"
                    else if (settings.libraryLayout === "grid") settings.libraryLayout = "list"
                    else settings.libraryLayout = "ribbon"
                }
            }
        }

        // Collections Strip (LT/RT)
        Row {
            visible: library.collections.length > 0
            spacing: 8

            Text {
                text: "Collections:"
                color: Qt.rgba(0.80, 0.88, 1.0, 0.65)
                font.family: settings.fontFamily
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }

            FocusButton {
                text: "All"
                selected: library.collectionFilter === ""
                implicitHeight: 30
                implicitWidth: 64
                buttonRadius: 15
                fontSize: 12
                onClicked: library.collectionFilter = ""
            }

            Repeater {
                model: library.collections
                FocusButton {
                    text: modelData.name + " (" + modelData.gameCount + ")"
                    selected: library.collectionFilter === modelData.id
                    implicitHeight: 30
                    buttonRadius: 15
                    fontSize: 12
                    onClicked: library.collectionFilter = modelData.id
                }
            }
        }

        // Cinematic Hero Panel for currently selected game (PS5 / Big Picture showcase)
        GlassPanel {
            visible: home.currentGame !== null && settings.libraryLayout === "ribbon"
            Layout.fillWidth: true
            Layout.preferredHeight: 196
            glassRadius: 24
            elevated: true
            opacityBoost: 0.08

            RowLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Row {
                        spacing: 10
                        Rectangle {
                            height: 24
                            width: heroPlat.implicitWidth + 18
                            radius: 12
                            color: Qt.rgba(settings.accentColor.r, settings.accentColor.g, settings.accentColor.b, 0.25)
                            border.width: 1
                            border.color: Qt.rgba(settings.accentColor.r, settings.accentColor.g, settings.accentColor.b, 0.6)
                            Text {
                                id: heroPlat
                                anchors.centerIn: parent
                                text: home.currentGame ? home.currentGame.platform : ""
                                color: "#FFFFFF"
                                font.family: settings.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.Bold
                            }
                        }

                        Text {
                            visible: home.currentGame && home.currentGame.releaseYear.length > 0
                            text: home.currentGame ? home.currentGame.releaseYear : ""
                            color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                            font.family: settings.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            visible: home.currentGame && home.currentGame.developer.length > 0
                            text: home.currentGame ? ("• " + home.currentGame.developer) : ""
                            color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                            font.family: settings.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: home.currentGame ? home.currentGame.title : ""
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 32
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: (home.currentGame && home.currentGame.description.length > 0)
                            ? home.currentGame.description
                            : "Ready to launch. Open Game Details to customize artwork, metadata, collections, or per-game launch parameters."
                        color: Qt.rgba(0.86, 0.92, 1.0, 0.78)
                        font.family: settings.fontFamily
                        font.pixelSize: 14
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        wrapMode: Text.WordWrap
                    }

                    Row {
                        spacing: 12
                        FocusButton {
                            text: "Play Now"
                            iconText: "▶"
                            primary: true
                            implicitHeight: 42
                            onClicked: {
                                if (home.currentGame) home.launchGameRequested(home.currentGame.id)
                            }
                        }
                        FocusButton {
                            text: "Game Page"
                            implicitHeight: 42
                            onClicked: {
                                if (home.currentGame) home.openGameDetails(home.currentGame.id)
                            }
                        }
                        FocusButton {
                            text: (home.currentGame && home.currentGame.isFavorite) ? "★ Favorited" : "☆ Favorite"
                            implicitHeight: 42
                            onClicked: home.toggleCurrentFavorite()
                        }
                    }
                }

                // Right-side playtime telemetry card
                ColumnLayout {
                    Layout.preferredWidth: 210
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 10

                    GlassPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        glassRadius: 16
                        Column {
                            anchors.centerIn: parent
                            spacing: 2
                            Text {
                                text: "PLAYTIME"
                                color: Qt.rgba(0.78, 0.86, 1.0, 0.62)
                                font.family: settings.fontFamily
                                font.pixelSize: 10
                                font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: home.currentGame ? home.currentGame.formattedPlaytime : "—"
                                color: "#FFFFFF"
                                font.family: settings.fontFamily
                                font.pixelSize: 16
                                font.weight: Font.Bold
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }

                    GlassPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        glassRadius: 16
                        Column {
                            anchors.centerIn: parent
                            spacing: 2
                            Text {
                                text: "LAST PLAYED"
                                color: Qt.rgba(0.78, 0.86, 1.0, 0.62)
                                font.family: settings.fontFamily
                                font.pixelSize: 10
                                font.bold: true
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: home.currentGame ? home.currentGame.formattedLastPlayed : "Never"
                                color: "#FFFFFF"
                                font.family: settings.fontFamily
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }
        }

        // Main Game Showcase Container (Ribbon / Grid / List / Empty State)
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 1. PS5-Style Horizontal Ribbon
            ListView {
                id: ribbonView
                anchors.fill: parent
                visible: home.gamesModel.length > 0 && settings.libraryLayout === "ribbon"
                orientation: ListView.Horizontal
                spacing: 22
                clip: false
                leftMargin: 6
                rightMargin: 40
                model: home.gamesModel
                currentIndex: home.selectedIndex
                highlightMoveDuration: settings.animationsEnabled ? Math.round(240 / settings.animationSpeed) : 0

                delegate: GameCard {
                    width: 215
                    height: Math.min(310, Math.max(220, ribbonView.height - 20))
                    anchors.verticalCenter: parent.verticalCenter
                    gameData: modelData
                    isCurrent: index === home.selectedIndex
                    onSelected: home.selectedIndex = index
                    onDetailsRequested: home.openGameDetails(modelData.id)
                    onActivated: home.launchGameRequested(modelData.id)
                }
            }

            // 2. Glass Grid View
            GridView {
                id: gridView
                anchors.fill: parent
                visible: home.gamesModel.length > 0 && settings.libraryLayout === "grid"
                clip: true
                cellWidth: 230
                cellHeight: 320
                model: home.gamesModel
                currentIndex: home.selectedIndex

                delegate: Item {
                    width: gridView.cellWidth
                    height: gridView.cellHeight
                    GameCard {
                        anchors.fill: parent
                        anchors.margins: 10
                        gameData: modelData
                        isCurrent: index === home.selectedIndex
                        onSelected: home.selectedIndex = index
                        onDetailsRequested: home.openGameDetails(modelData.id)
                        onActivated: home.launchGameRequested(modelData.id)
                    }
                }
            }

            // 3. Compact List View
            ListView {
                id: listView
                anchors.fill: parent
                visible: home.gamesModel.length > 0 && settings.libraryLayout === "list"
                clip: true
                spacing: 10
                model: home.gamesModel
                currentIndex: home.selectedIndex

                delegate: GameCard {
                    width: listView.width
                    height: 76
                    compactList: true
                    gameData: modelData
                    isCurrent: index === home.selectedIndex
                    onSelected: home.selectedIndex = index
                    onDetailsRequested: home.openGameDetails(modelData.id)
                    onActivated: home.launchGameRequested(modelData.id)
                }
            }

            // Clean Empty Library / Empty Filter Onboarding Card (Zero fake games!)
            GlassPanel {
                visible: home.gamesModel.length === 0
                anchors.centerIn: parent
                width: Math.min(parent.width - 40, 740)
                height: 310
                glassRadius: 28
                elevated: true
                opacityBoost: 0.10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 34
                    spacing: 14

                    Text {
                        text: library.totalGamesCount === 0
                            ? "Welcome to Aether Launcher"
                            : "No Games Match This Filter"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 28
                        font.weight: Font.Bold
                    }

                    Text {
                        Layout.fillWidth: true
                        text: library.totalGamesCount === 0
                            ? "Your liquid glass Windows 11 console dashboard is ready. Configure Lichii for Nintendo Switch 2, add standalone Windows executables (.exe), or scan your existing game directories to populate your library."
                            : "Try clearing your search query, switching sections with LB/RB, or selecting All Platforms."
                        color: Qt.rgba(0.85, 0.92, 1.0, 0.80)
                        font.family: settings.fontFamily
                        font.pixelSize: 15
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.fillHeight: true }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 12

                        FocusButton {
                            visible: library.totalGamesCount === 0
                            text: "Configure Lichii (Switch 2)"
                            primary: true
                            onClicked: home.openEmulatorsRequested()
                        }
                        FocusButton {
                            visible: library.totalGamesCount === 0
                            text: "Add Windows Game (.exe)"
                            onClicked: home.addExecutableRequested()
                        }
                        FocusButton {
                            visible: library.totalGamesCount === 0
                            text: "Add ROM / Game File"
                            onClicked: home.addRomRequested()
                        }
                        FocusButton {
                            visible: library.totalGamesCount === 0
                            text: "Scan Game Folder"
                            onClicked: home.scanFolderRequested()
                        }
                        FocusButton {
                            visible: library.totalGamesCount > 0
                            text: "Clear All Filters"
                            primary: true
                            onClicked: {
                                library.activeSection = "all"
                                library.platformFilter = ""
                                library.collectionFilter = ""
                                library.searchQuery = ""
                                searchInput.text = ""
                                platformBox.currentIndex = 0
                            }
                        }
                    }
                }
            }
        }
    }
}
