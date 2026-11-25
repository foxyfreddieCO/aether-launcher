import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import "components"
import "views"
import "widgets"

ApplicationWindow {
    id: win
    visible: true
    width: 1440
    height: 860
    minimumWidth: 1024
    minimumHeight: 640
    title: "Aether Launcher"
    color: "#060911"

    property string currentPage: "home" // home, detail, emulators, library, settings
    property string activeDetailGameId: ""
    property bool quickMenuOpen: false
    property bool screensaverActive: false
    property string toastMessage: ""

    function showToast(msg) {
        if (!msg || msg.length === 0) return
        toastMessage = msg
        toastTimer.restart()
    }

    function applyDisplaySettings() {
        sys.applyWindowStyle(win)
        sys.moveWindowToMonitor(win, settings.targetMonitorIndex, settings.fullscreen)
    }

    function resetIdleTimer() {
        if (screensaverActive) {
            screensaverActive = false
        }
        if (settings.screensaverEnabled && !launcher.isRunning) {
            idleTimer.restart()
        } else {
            idleTimer.stop()
        }
    }

    function openPage(page) {
        currentPage = page
        resetIdleTimer()
    }

    function openGamePage(gameId) {
        activeDetailGameId = gameId
        currentPage = "detail"
        detailView.refresh()
        resetIdleTimer()
    }

    function launchGameById(gameId) {
        audio.playLaunch()
        controller.rumble(0.35, 0.55, 160)
        launcher.launchGame(gameId)
    }

    Component.onCompleted: {
        applyDisplaySettings()
        resetIdleTimer()
    }

    // Idle Screensaver Timer
    Timer {
        id: idleTimer
        interval: Math.max(1, settings.screensaverTimeoutMinutes) * 60000
        repeat: false
        onTriggered: {
            if (settings.screensaverEnabled && !launcher.isRunning) {
                win.screensaverActive = true
            }
        }
    }

    Timer {
        id: toastTimer
        interval: 4200
        onTriggered: win.toastMessage = ""
    }

    // Quick Onboarding File/Folder Dialogs from Home empty state
    FileDialog {
        id: quickExeDialog
        title: "Add Windows Executable Game"
        nameFilters: ["Executables (*.exe *.bat *.cmd *.lnk *.app *)", "All Files (*)"]
        onAccepted: library.addManualExecutable(selectedFile.toString())
    }

    FileDialog {
        id: quickRomDialog
        title: "Add ROM / Game File"
        onAccepted: library.addGameFile(selectedFile.toString())
    }

    FolderDialog {
        id: quickScanDialog
        title: "Select Game Directory to Scan"
        onAccepted: library.scanDirectory(selectedFolder.toString())
    }

    // Backend Signal Connections
    Connections {
        target: controller
        function onUserActivity() { win.resetIdleTimer() }
        function onControllerConnected(name) { win.showToast("Controller connected: " + name) }
        function onControllerDisconnected(name) { win.showToast("Controller disconnected: " + name) }

        function onNavigate(direction) {
            if (win.screensaverActive) { win.resetIdleTimer(); return }
            if (launchOverlay.errorVisible) return
            if (win.quickMenuOpen) return
            if (win.currentPage === "home") {
                homeView.handleNavigate(direction)
            } else if (win.currentPage === "settings") {
                if (direction === "up") settingsView.cycleTab(-1)
                else if (direction === "down") settingsView.cycleTab(1)
            }
        }

        function onActionTriggered(action) {
            if (win.screensaverActive) { win.resetIdleTimer(); return }

            if (launchOverlay.errorVisible) {
                if (action === "back" || action === "confirm") {
                    audio.playBack()
                    launchOverlay.errorVisible = false
                }
                return
            }

            if (action === "quickMenu") {
                win.quickMenuOpen = !win.quickMenuOpen
                audio.playConfirm()
                return
            }

            if (win.quickMenuOpen) {
                if (action === "back") {
                    audio.playBack()
                    win.quickMenuOpen = false
                }
                return
            }

            if (win.currentPage === "home") {
                if (action === "confirm") {
                    homeView.activateCurrent()
                } else if (action === "info") {
                    if (homeView.currentGame) win.openGamePage(homeView.currentGame.id)
                } else if (action === "options") {
                    homeView.toggleCurrentFavorite()
                } else if (action === "prevCategory") {
                    audio.playNavigate()
                    library.cycleSection(-1)
                } else if (action === "nextCategory") {
                    audio.playNavigate()
                    library.cycleSection(1)
                } else if (action === "prevCollection") {
                    audio.playNavigate()
                    library.cycleCollection(-1)
                } else if (action === "nextCollection") {
                    audio.playNavigate()
                    library.cycleCollection(1)
                }
            } else if (win.currentPage === "detail") {
                if (action === "confirm") {
                    win.launchGameById(win.activeDetailGameId)
                } else if (action === "back") {
                    audio.playBack()
                    win.openPage("home")
                } else if (action === "options") {
                    detailView.editExpanded = !detailView.editExpanded
                    audio.playConfirm()
                }
            } else if (win.currentPage === "settings") {
                if (action === "back") {
                    audio.playBack()
                    win.openPage("home")
                } else if (action === "prevCategory") {
                    settingsView.cycleTab(-1)
                } else if (action === "nextCategory") {
                    settingsView.cycleTab(1)
                }
            } else {
                if (action === "back") {
                    audio.playBack()
                    win.openPage("home")
                }
            }
        }
    }

    Connections {
        target: launcher
        function onGameLaunched(gameId, title) {
            controller.inputSuspended = true
            audio.suspendForGame()
            idleTimer.stop()
        }
        function onGameExited(gameId, title, seconds, exitCode) {
            controller.inputSuspended = false
            audio.resumeAfterGame()
            audio.playExit()
            library.reload()
            win.raise()
            win.requestActivate()
            win.resetIdleTimer()
            win.showToast("Session saved for " + title + " (" + library.formatPlaytime(seconds) + ")")
        }
        function onLaunchFailed(title, reason) {
            controller.inputSuspended = false
            audio.resumeAfterGame()
            audio.playError()
            launchOverlay.showError(title, reason)
        }
    }

    Connections {
        target: library
        function onStatusMessageChanged(msg) { win.showToast(msg) }
    }

    Connections {
        target: updater
        function onUpdateAvailableNotified(version, notes) {
            win.showToast("Aether Launcher v" + version + " is available! Open Settings -> Updates to install.")
        }
    }

    // Dynamic Background (follows selected game artwork or custom wallpaper)
    AmbientBackground {
        anchors.fill: parent
        imageSource: {
            if (settings.useDynamicGameBackground) {
                var target = (win.currentPage === "detail" && detailView.game && detailView.game.id)
                    ? detailView.game
                    : homeView.currentGame
                if (target) {
                    if ((target.backgroundPath || "").length > 0) return "file://" + target.backgroundPath
                    if ((target.coverPath || "").length > 0) return "file://" + target.coverPath
                }
            }
            if ((settings.wallpaperPath || "").length > 0) return "file://" + settings.wallpaperPath
            return ""
        }
    }

    // Global Keyboard Navigation & Activity Capture
    Item {
        id: keyHandler
        anchors.fill: parent
        focus: true

        Keys.onPressed: function(event) {
            controller.notifyKeyboardMouseActivity()
            win.resetIdleTimer()

            if (event.key === Qt.Key_F11) {
                settings.fullscreen = !settings.fullscreen
                win.applyDisplaySettings()
                event.accepted = true
                return
            }

            if (event.key === Qt.Key_Tab) {
                win.quickMenuOpen = !win.quickMenuOpen
                audio.playConfirm()
                event.accepted = true
                return
            }

            if (event.key === Qt.Key_Escape) {
                if (launchOverlay.errorVisible) {
                    launchOverlay.errorVisible = false
                } else if (win.quickMenuOpen) {
                    audio.playBack()
                    win.quickMenuOpen = false
                } else if (win.currentPage !== "home") {
                    audio.playBack()
                    win.openPage("home")
                }
                event.accepted = true
                return
            }

            if (win.quickMenuOpen || launchOverlay.visible) return

            if (win.currentPage === "home") {
                if (event.key === Qt.Key_Left) { homeView.handleNavigate("left"); event.accepted = true }
                else if (event.key === Qt.Key_Right) { homeView.handleNavigate("right"); event.accepted = true }
                else if (event.key === Qt.Key_Up) { homeView.handleNavigate("up"); event.accepted = true }
                else if (event.key === Qt.Key_Down) { homeView.handleNavigate("down"); event.accepted = true }
                else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) { homeView.activateCurrent(); event.accepted = true }
                else if (event.key === Qt.Key_Q) { audio.playNavigate(); library.cycleSection(-1); event.accepted = true }
                else if (event.key === Qt.Key_E) { audio.playNavigate(); library.cycleSection(1); event.accepted = true }
                else if (event.key === Qt.Key_BracketLeft) { audio.playNavigate(); library.cycleCollection(-1); event.accepted = true }
                else if (event.key === Qt.Key_BracketRight) { audio.playNavigate(); library.cycleCollection(1); event.accepted = true }
            }
        }
    }

    // Main Application Chrome + View Area
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top Liquid Glass Bar (Nav + Customizable Widgets)
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 68

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 32
                anchors.rightMargin: 32
                anchors.topMargin: 14
                anchors.bottomMargin: 6
                spacing: 14

                // Brand Emblem
                Row {
                    spacing: 10
                    Rectangle {
                        width: 34
                        height: 34
                        radius: 12
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: settings.accentColor }
                            GradientStop { position: 1.0; color: settings.secondaryAccent }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "A"
                            color: "#FFFFFF"
                            font.family: settings.fontFamily
                            font.pixelSize: 18
                            font.weight: Font.Black
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: -2
                        Text {
                            text: "AETHER"
                            color: "#FFFFFF"
                            font.family: settings.fontFamily
                            font.pixelSize: 15
                            font.weight: Font.Black
                            font.letterSpacing: 1.4
                        }
                        Text {
                            text: settings.consoleMode ? "CONSOLE" : "DESKTOP"
                            color: settings.accentColor
                            font.family: settings.fontFamily
                            font.pixelSize: 9
                            font.bold: true
                            font.letterSpacing: 1.2
                        }
                    }
                }

                // Primary Nav Pills
                Row {
                    spacing: 8
                    FocusButton {
                        text: "Home"
                        selected: win.currentPage === "home" || win.currentPage === "detail"
                        implicitWidth: 82
                        implicitHeight: 38
                        buttonRadius: 19
                        fontSize: 13
                        onClicked: win.openPage("home")
                    }
                    FocusButton {
                        text: "Emulators"
                        selected: win.currentPage === "emulators"
                        implicitWidth: 106
                        implicitHeight: 38
                        buttonRadius: 19
                        fontSize: 13
                        onClicked: win.openPage("emulators")
                    }
                    FocusButton {
                        text: "Library"
                        selected: win.currentPage === "library"
                        implicitWidth: 88
                        implicitHeight: 38
                        buttonRadius: 19
                        fontSize: 13
                        onClicked: win.openPage("library")
                    }
                    FocusButton {
                        text: "Settings"
                        selected: win.currentPage === "settings"
                        implicitWidth: 94
                        implicitHeight: 38
                        buttonRadius: 19
                        fontSize: 13
                        onClicked: win.openPage("settings")
                    }
                }

                // Customizable Widget Bar
                WidgetBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    onOpenQuickMenu: win.quickMenuOpen = true
                    onOpenRecentGame: function(gid) { win.openGamePage(gid) }
                }

                // Quick Menu Trigger
                FocusButton {
                    text: "Quick Menu"
                    implicitHeight: 38
                    buttonRadius: 19
                    fontSize: 12
                    selected: win.quickMenuOpen
                    onClicked: win.quickMenuOpen = !win.quickMenuOpen
                }
            }
        }

        // Main Content Pages
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            HomeView {
                id: homeView
                anchors.fill: parent
                visible: win.currentPage === "home"
                onOpenGameDetails: function(gid) { win.openGamePage(gid) }
                onLaunchGameRequested: function(gid) { win.launchGameById(gid) }
                onOpenEmulatorsRequested: win.openPage("emulators")
                onOpenLibraryManagerRequested: win.openPage("library")
                onAddExecutableRequested: quickExeDialog.open()
                onAddRomRequested: quickRomDialog.open()
                onScanFolderRequested: quickScanDialog.open()
            }

            GameDetailView {
                id: detailView
                anchors.fill: parent
                visible: win.currentPage === "detail"
                gameId: win.activeDetailGameId
                onBackRequested: win.openPage("home")
                onLaunchRequested: function(gid) { win.launchGameById(gid) }
            }

            EmulatorsView {
                id: emulatorsView
                anchors.fill: parent
                visible: win.currentPage === "emulators"
                onBackRequested: win.openPage("home")
            }

            LibraryManagerView {
                id: libraryView
                anchors.fill: parent
                visible: win.currentPage === "library"
                onBackRequested: win.openPage("home")
                onOpenGameDetails: function(gid) { win.openGamePage(gid) }
            }

            SettingsView {
                id: settingsView
                anchors.fill: parent
                visible: win.currentPage === "settings"
                onBackRequested: win.openPage("home")
                onDisplayModeChanged: win.applyDisplaySettings()
            }
        }

        // Bottom Controller / Keyboard Glyph Hint Bar
        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            Layout.leftMargin: 28
            Layout.rightMargin: 28
            Layout.bottomMargin: 12
            glassRadius: 16
            opacityBoost: 0.04

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 22

                GlyphBadge { actionName: "confirm"; label: win.currentPage === "detail" ? "Play" : "Select" }
                GlyphBadge { actionName: "back"; label: "Back"; visible: win.currentPage !== "home" || win.quickMenuOpen }
                GlyphBadge { actionName: "info"; label: "Game Page"; visible: win.currentPage === "home" && homeView.currentGame !== null }
                GlyphBadge { actionName: "options"; label: win.currentPage === "detail" ? "Game Settings" : "Favorite"; visible: (win.currentPage === "home" && homeView.currentGame !== null) || win.currentPage === "detail" }
                GlyphBadge { actionName: "prevCategory"; label: "Section"; visible: win.currentPage === "home" }
                GlyphBadge { actionName: "nextCollection"; label: "Collections"; visible: win.currentPage === "home" && library.collections.length > 0 }

                Item { Layout.fillWidth: true }

                GlyphBadge { actionName: "quickMenu"; label: "Quick Menu" }
            }
        }
    }

    // Floating Performance Monitoring Overlay HUD
    GlassPanel {
        visible: settings.performanceOverlay
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 76
        anchors.rightMargin: 24
        implicitWidth: hudRow.implicitWidth + 28
        height: 36
        glassRadius: 14
        elevated: true
        opacityBoost: 0.30
        z: 90

        Row {
            id: hudRow
            anchors.centerIn: parent
            spacing: 14
            Text { text: "CPU " + Math.round(sys.cpuUsage) + "%"; color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
            Text { visible: sys.gpuAvailable; text: "GPU " + Math.round(sys.gpuUsage) + "%"; color: settings.accentColor; font.pixelSize: 12; font.bold: true }
            Text { text: "RAM " + sys.ramUsedGb.toFixed(1) + "G (" + Math.round(sys.ramUsage) + "%)"; color: "#D2E4FF"; font.pixelSize: 12; font.bold: true }
            Text { visible: sys.temperatureAvailable; text: Math.round(sys.temperatureC) + "°C"; color: "#FBBF24"; font.pixelSize: 12; font.bold: true }
        }
    }

    // Non-Intrusive Toast Notification Banner
    GlassPanel {
        visible: win.toastMessage.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 72
        width: Math.min(parent.width - 80, toastLbl.implicitWidth + 44)
        height: 44
        glassRadius: 22
        elevated: true
        opacityBoost: 0.35
        z: 150

        Text {
            id: toastLbl
            anchors.centerIn: parent
            text: win.toastMessage
            color: "#FFFFFF"
            font.family: settings.fontFamily
            font.pixelSize: 13
            font.weight: Font.Medium
        }
    }

    // Quick Menu Slide-Over Drawer
    QuickMenuOverlay {
        anchors.fill: parent
        isOpen: win.quickMenuOpen
        onCloseRequested: win.quickMenuOpen = false
        onNavigateTo: function(page) { win.openPage(page) }
        onDisplayModeChanged: win.applyDisplaySettings()
        onQuitRequested: Qt.quit()
    }

    // Cinematic Game Launch & Error Overlay
    LaunchOverlay {
        id: launchOverlay
        onOpenEmulatorsRequested: win.openPage("emulators")
    }

    // Custom Ambient Idle Screensaver Overlay
    Item {
        visible: win.screensaverActive
        anchors.fill: parent
        z: 300

        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.02, 0.03, 0.06, 0.86)
        }

        Column {
            anchors.centerIn: parent
            spacing: 12

            Text {
                text: sys.currentTime
                color: "#FFFFFF"
                font.family: settings.fontFamily
                font.pixelSize: 84
                font.weight: Font.Light
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                text: sys.currentDate
                color: settings.accentColor
                font.family: settings.fontFamily
                font.pixelSize: 22
                font.weight: Font.Medium
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                text: "Press any button or move mouse to return to Aether Launcher"
                color: Qt.rgba(0.84, 0.90, 1.0, 0.55)
                font.family: settings.fontFamily
                font.pixelSize: 13
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onPositionChanged: win.resetIdleTimer()
            onClicked: win.resetIdleTimer()
        }
    }
}
