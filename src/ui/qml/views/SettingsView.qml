import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Item {
    id: settingsView

    property int activeTab: 0
    readonly property var tabs: [
        "Appearance & Themes",
        "Audio & Atmosphere",
        "Controller & Input",
        "Widgets",
        "System & Display",
        "Updates & Rollback"
    ]

    signal backRequested()
    signal displayModeChanged()

    FileDialog {
        id: wallpaperPicker
        title: "Choose Custom Wallpaper"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: settings.wallpaperPath = library.normalizeFileUrl(selectedFile.toString())
    }

    FolderDialog {
        id: soundPackPicker
        title: "Select Custom Sound Pack Directory"
        onAccepted: {
            settings.customSoundPackDir = library.normalizeFileUrl(selectedFolder.toString())
            audio.reloadSoundPack()
        }
    }

    FileDialog {
        id: bgmPicker
        title: "Select Ambient Background Music File"
        nameFilters: ["Audio (*.mp3 *.wav *.ogg *.flac *.m4a)"]
        onAccepted: settings.customBgmPath = library.normalizeFileUrl(selectedFile.toString())
    }

    function cycleTab(delta) {
        activeTab = (activeTab + delta + tabs.length) % tabs.length
        audio.playNavigate()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 22

        // Left Controller-Friendly Category Sidebar
        GlassPanel {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            glassRadius: 24
            opacityBoost: 0.10

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                FocusButton {
                    Layout.fillWidth: true
                    text: "← Back to Launcher"
                    onClicked: {
                        audio.playBack()
                        settingsView.backRequested()
                    }
                }

                Text {
                    text: "SETTINGS"
                    color: Qt.rgba(0.76, 0.86, 1.0, 0.60)
                    font.family: settings.fontFamily
                    font.pixelSize: 11
                    font.bold: true
                    Layout.topMargin: 8
                }

                Repeater {
                    model: settingsView.tabs
                    FocusButton {
                        Layout.fillWidth: true
                        text: modelData
                        selected: settingsView.activeTab === index
                        onClicked: settingsView.activeTab = index
                    }
                }

                Item { Layout.fillHeight: true }

                Text {
                    text: "Aether Launcher v" + updater.currentVersion + "\n" + sys.platformName
                    color: Qt.rgba(0.76, 0.86, 1.0, 0.55)
                    font.family: settings.fontFamily
                    font.pixelSize: 11
                }
            }
        }

        // Right Active Settings Content
        GlassPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            glassRadius: 24
            opacityBoost: 0.08

            Flickable {
                anchors.fill: parent
                anchors.margins: 26
                contentHeight: tabStack.implicitHeight + 36
                clip: true

                ColumnLayout {
                    id: tabStack
                    width: parent.width
                    spacing: 18

                    // TAB 0: APPEARANCE & THEMES
                    ColumnLayout {
                        visible: settingsView.activeTab === 0
                        Layout.fillWidth: true
                        spacing: 16

                        Text { text: "Appearance & Liquid Glass Themes"; color: "#FFFFFF"; font.family: settings.fontFamily; font.pixelSize: 24; font.bold: true }

                        // Built-in & Custom Themes
                        Text { text: "Themes"; color: "#B8CCE8"; font.pixelSize: 13; font.bold: true }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 10
                            Repeater {
                                model: settings.themes
                                Row {
                                    spacing: 4
                                    FocusButton {
                                        text: modelData.name
                                        selected: settings.activeThemeId === modelData.id
                                        implicitHeight: 38
                                        onClicked: settings.applyTheme(modelData.id)
                                    }
                                    FocusButton {
                                        visible: !modelData.isBuiltIn
                                        text: "✕"
                                        danger: true
                                        implicitWidth: 34
                                        implicitHeight: 38
                                        onClicked: settings.deleteCustomTheme(modelData.id)
                                    }
                                }
                            }
                        }

                        // Save Custom Theme
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            TextField {
                                id: customThemeName
                                Layout.preferredWidth: 260
                                placeholderText: "Save current look as custom theme..."
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 12; opacityBoost: 0.1 }
                            }
                            FocusButton {
                                text: "Save Theme"
                                primary: true
                                implicitHeight: 38
                                onClicked: {
                                    if (customThemeName.text.trim().length > 0) {
                                        settings.saveCustomTheme(customThemeName.text)
                                        customThemeName.text = ""
                                    }
                                }
                            }
                        }

                        // Accent Color Swatches
                        Text { text: "Accent Color"; color: "#B8CCE8"; font.pixelSize: 13; font.bold: true }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 10
                            Repeater {
                                model: ["#4CC2FF", "#8B6DFF", "#2EE6B8", "#FF5C7A", "#FF9E44", "#FACC15", "#E2E8F0", "#38BDF8"]
                                Rectangle {
                                    width: 36
                                    height: 36
                                    radius: 18
                                    color: modelData
                                    border.width: settings.accentColor.toLowerCase() === modelData.toLowerCase() ? 3 : 1
                                    border.color: settings.accentColor.toLowerCase() === modelData.toLowerCase() ? "#FFFFFF" : Qt.rgba(1,1,1,0.3)
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            audio.playConfirm()
                                            settings.accentColor = modelData
                                        }
                                    }
                                }
                            }
                        }

                        // Sliders: Glass Opacity, Blur Intensity, Animation Speed
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 20
                            rowSpacing: 14

                            Text { text: "Liquid Glass Opacity (" + Math.round(settings.glassOpacity * 100) + "%)"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0.08; to: 0.75; value: settings.glassOpacity
                                onMoved: settings.glassOpacity = value
                            }

                            Text { text: "Backdrop Blur Intensity (" + Math.round(settings.blurIntensity * 100) + "%)"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0.0; to: 1.0; value: settings.blurIntensity
                                onMoved: settings.blurIntensity = value
                            }

                            Text { text: "UI Animations"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.animationsEnabled ? "Enabled" : "Disabled"
                                selected: settings.animationsEnabled
                                implicitHeight: 36
                                onClicked: settings.animationsEnabled = !settings.animationsEnabled
                            }

                            Text { text: "Animation Speed (" + settings.animationSpeed.toFixed(1) + "x)"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0.5; to: 2.0; stepSize: 0.1; value: settings.animationSpeed
                                onMoved: settings.animationSpeed = value
                            }

                            Text { text: "Dynamic Game Backgrounds"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.useDynamicGameBackground ? "Follow Selected Game" : "Static Wallpaper Only"
                                selected: settings.useDynamicGameBackground
                                implicitHeight: 36
                                onClicked: settings.useDynamicGameBackground = !settings.useDynamicGameBackground
                            }

                            Text { text: "Custom Static Wallpaper"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Row {
                                spacing: 8
                                FocusButton { text: "Choose Wallpaper..."; implicitHeight: 36; onClicked: wallpaperPicker.open() }
                                FocusButton {
                                    visible: settings.wallpaperPath.length > 0
                                    text: "Clear"
                                    danger: true
                                    implicitHeight: 36
                                    onClicked: settings.wallpaperPath = ""
                                }
                            }

                            Text { text: "Typography Font"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Row {
                                spacing: 8
                                Repeater {
                                    model: ["Segoe UI Variable", "Segoe UI", "Inter", "SF Pro Display", "Bahnschrift"]
                                    FocusButton {
                                        text: modelData
                                        selected: settings.fontFamily === modelData
                                        implicitHeight: 34
                                        fontSize: 12
                                        onClicked: settings.fontFamily = modelData
                                    }
                                }
                            }

                            Text { text: "Default Library Layout"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Row {
                                spacing: 8
                                Repeater {
                                    model: [
                                        { id: "ribbon", label: "Console Ribbon" },
                                        { id: "grid", label: "Glass Grid" },
                                        { id: "list", label: "Compact List" }
                                    ]
                                    FocusButton {
                                        text: modelData.label
                                        selected: settings.libraryLayout === modelData.id
                                        implicitHeight: 36
                                        onClicked: settings.libraryLayout = modelData.id
                                    }
                                }
                            }
                        }
                    }

                    // TAB 1: AUDIO & ATMOSPHERE
                    ColumnLayout {
                        visible: settingsView.activeTab === 1
                        Layout.fillWidth: true
                        spacing: 16

                        Text { text: "Sound Effects & Ambient Atmosphere"; color: "#FFFFFF"; font.family: settings.fontFamily; font.pixelSize: 24; font.bold: true }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 20
                            rowSpacing: 14

                            Text { text: "Master Launcher Mute"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.audioMuted ? "Muted" : "Active"
                                selected: !settings.audioMuted
                                implicitHeight: 36
                                onClicked: settings.audioMuted = !settings.audioMuted
                            }

                            Text { text: "UI Navigation & Launch SFX"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.sfxEnabled ? "Enabled" : "Disabled"
                                selected: settings.sfxEnabled
                                implicitHeight: 36
                                onClicked: settings.sfxEnabled = !settings.sfxEnabled
                            }

                            Text { text: "SFX Volume (" + Math.round(settings.sfxVolume * 100) + "%)"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0.0; to: 1.0; value: settings.sfxVolume
                                onMoved: settings.sfxVolume = value
                            }

                            Text { text: "Ambient Background Music"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.bgmEnabled ? "Playing" : "Off"
                                selected: settings.bgmEnabled
                                implicitHeight: 36
                                onClicked: settings.bgmEnabled = !settings.bgmEnabled
                            }

                            Text { text: "Music Volume (" + Math.round(settings.bgmVolume * 100) + "%)"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Slider {
                                Layout.fillWidth: true
                                from: 0.0; to: 1.0; value: settings.bgmVolume
                                onMoved: settings.bgmVolume = value
                            }

                            Text { text: "Custom Sound Pack Folder"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Row {
                                spacing: 8
                                FocusButton { text: "Choose Sound Pack..."; implicitHeight: 36; onClicked: soundPackPicker.open() }
                                FocusButton {
                                    visible: settings.customSoundPackDir.length > 0
                                    text: "Reset to Default"
                                    implicitHeight: 36
                                    onClicked: { settings.customSoundPackDir = ""; audio.reloadSoundPack() }
                                }
                            }

                            Text { text: "Custom Ambient Music File"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Row {
                                spacing: 8
                                FocusButton { text: "Choose Audio Track..."; implicitHeight: 36; onClicked: bgmPicker.open() }
                                FocusButton {
                                    visible: settings.customBgmPath.length > 0
                                    text: "Use Built-In Pad"
                                    implicitHeight: 36
                                    onClicked: settings.customBgmPath = ""
                                }
                            }
                        }

                        Row {
                            spacing: 10
                            FocusButton { text: "Preview Navigate"; implicitHeight: 36; onClicked: audio.playNavigate() }
                            FocusButton { text: "Preview Confirm"; implicitHeight: 36; onClicked: audio.playConfirm() }
                            FocusButton { text: "Preview Back"; implicitHeight: 36; onClicked: audio.playBack() }
                            FocusButton { text: "Preview Launch"; implicitHeight: 36; onClicked: audio.playLaunch() }
                        }
                    }

                    // TAB 2: CONTROLLER & INPUT
                    ColumnLayout {
                        visible: settingsView.activeTab === 2
                        Layout.fillWidth: true
                        spacing: 16

                        Text { text: "Controller Support & Button Remapping"; color: "#FFFFFF"; font.family: settings.fontFamily; font.pixelSize: 24; font.bold: true }

                        GlassPanel {
                            Layout.fillWidth: true
                            implicitHeight: ctrlStatusCol.implicitHeight + 30
                            glassRadius: 16
                            opacityBoost: 0.10

                            ColumnLayout {
                                id: ctrlStatusCol
                                anchors.fill: parent
                                anchors.margins: 15
                                spacing: 6

                                Text {
                                    text: controller.connected
                                        ? ("Connected: " + controller.primaryName + " (" + controller.controllerFamily.toUpperCase() + ")")
                                        : "No controller detected yet. Pair via Bluetooth or connect USB (Xbox / PlayStation DualSense / DualShock)."
                                    color: controller.connected ? "#6EE7B7" : "#DCE7F7"
                                    font.family: settings.fontFamily
                                    font.pixelSize: 14
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }

                                Text {
                                    visible: controller.connected
                                    text: "Battery: " + (controller.batteryPercent >= 0 ? (controller.batteryPercent + "% (" + controller.batteryState + ")") : controller.batteryState)
                                        + "   •   Active Input Mode: " + controller.inputMode.toUpperCase()
                                    color: "#B8CCE8"
                                    font.pixelSize: 13
                                }
                            }
                        }

                        Row {
                            spacing: 12
                            FocusButton {
                                text: settings.rumbleEnabled ? "Controller Rumble: ON" : "Controller Rumble: OFF"
                                selected: settings.rumbleEnabled
                                onClicked: settings.rumbleEnabled = !settings.rumbleEnabled
                            }
                            FocusButton {
                                text: "Test Rumble"
                                enabledButton: controller.connected
                                onClicked: controller.testRumble()
                            }
                            FocusButton {
                                text: "Reset Default Mappings"
                                onClicked: settings.resetButtonMappings()
                            }
                        }

                        Text {
                            visible: controller.remappingAction.length > 0
                            text: "Press any button on your controller to assign to \"" + controller.remappingAction + "\" (or click Cancel)..."
                            color: "#FDE047"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Repeater {
                            model: [
                                { action: "confirm", label: "Confirm / Select (Default: A / Cross)" },
                                { action: "back", label: "Back / Cancel (Default: B / Circle)" },
                                { action: "info", label: "Game Info / Page (Default: X / Square)" },
                                { action: "options", label: "Game Options / Favorite (Default: Y / Triangle)" },
                                { action: "prevCategory", label: "Previous Category (Default: LB / L1)" },
                                { action: "nextCategory", label: "Next Category (Default: RB / R1)" },
                                { action: "prevCollection", label: "Previous Collection (Default: LT / L2)" },
                                { action: "nextCollection", label: "Next Collection (Default: RT / R2)" },
                                { action: "quickMenu", label: "Open Quick Menu (Default: Start / Options)" }
                            ]

                            GlassPanel {
                                Layout.fillWidth: true
                                implicitHeight: 46
                                glassRadius: 12
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 14
                                    anchors.rightMargin: 10

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.label
                                        color: "#E8F1FF"
                                        font.pixelSize: 13
                                    }

                                    Text {
                                        text: String(settings.buttonMappings[modelData.action] || "").toUpperCase()
                                        color: settings.accentColor
                                        font.pixelSize: 12
                                        font.bold: true
                                    }

                                    FocusButton {
                                        text: controller.remappingAction === modelData.action ? "Cancel" : "Remap"
                                        selected: controller.remappingAction === modelData.action
                                        implicitWidth: 88
                                        implicitHeight: 32
                                        fontSize: 12
                                        onClicked: {
                                            if (controller.remappingAction === modelData.action) controller.cancelRemap()
                                            else controller.beginRemap(modelData.action)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // TAB 3: WIDGETS
                    ColumnLayout {
                        visible: settingsView.activeTab === 3
                        Layout.fillWidth: true
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Customize Top Dashboard Widgets"; color: "#FFFFFF"; font.family: settings.fontFamily; font.pixelSize: 24; font.bold: true }
                            Item { Layout.fillWidth: true }
                            FocusButton { text: "Reset Defaults"; implicitHeight: 36; onClicked: settings.resetWidgets() }
                        }

                        Repeater {
                            model: settings.widgets
                            GlassPanel {
                                Layout.fillWidth: true
                                implicitHeight: 54
                                glassRadius: 14
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 16
                                    anchors.rightMargin: 12
                                    spacing: 10

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.title
                                        color: "#FFFFFF"
                                        font.family: settings.fontFamily
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }

                                    FocusButton {
                                        text: modelData.enabled ? "Visible" : "Hidden"
                                        selected: !!modelData.enabled
                                        implicitWidth: 84
                                        implicitHeight: 32
                                        fontSize: 12
                                        onClicked: settings.setWidgetEnabled(modelData.id, !modelData.enabled)
                                    }

                                    FocusButton {
                                        text: "Size: " + modelData.size
                                        implicitWidth: 112
                                        implicitHeight: 32
                                        fontSize: 12
                                        onClicked: {
                                            var next = modelData.size === "compact" ? "medium" : (modelData.size === "medium" ? "large" : "compact")
                                            settings.setWidgetSize(modelData.id, next)
                                        }
                                    }

                                    FocusButton {
                                        text: "◀"
                                        implicitWidth: 36
                                        implicitHeight: 32
                                        onClicked: settings.moveWidget(modelData.id, -1)
                                    }
                                    FocusButton {
                                        text: "▶"
                                        implicitWidth: 36
                                        implicitHeight: 32
                                        onClicked: settings.moveWidget(modelData.id, 1)
                                    }
                                }
                            }
                        }
                    }

                    // TAB 4: SYSTEM & DISPLAY
                    ColumnLayout {
                        visible: settingsView.activeTab === 4
                        Layout.fillWidth: true
                        spacing: 16

                        Text { text: "Windows 11 Integration & Display"; color: "#FFFFFF"; font.family: settings.fontFamily; font.pixelSize: 24; font.bold: true }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 20
                            rowSpacing: 14

                            Text { text: "Window Mode"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.fullscreen ? "Fullscreen Console" : "Windowed"
                                selected: settings.fullscreen
                                implicitHeight: 38
                                onClicked: {
                                    settings.fullscreen = !settings.fullscreen
                                    settingsView.displayModeChanged()
                                }
                            }

                            Text { text: "Launcher Mode"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.consoleMode ? "Console Mode (Big Picture)" : "Desktop Management Mode"
                                selected: settings.consoleMode
                                implicitHeight: 38
                                onClicked: settings.consoleMode = !settings.consoleMode
                            }

                            Text { text: "Target Display Monitor"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 8
                                Repeater {
                                    model: sys.monitors
                                    FocusButton {
                                        text: modelData.name + " (" + modelData.resolution + ")"
                                        selected: settings.targetMonitorIndex === modelData.index
                                        implicitHeight: 36
                                        fontSize: 12
                                        onClicked: {
                                            settings.targetMonitorIndex = modelData.index
                                            settingsView.displayModeChanged()
                                        }
                                    }
                                }
                            }

                            Text { text: "Performance Monitoring Overlay"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.performanceOverlay ? "Overlay Active" : "Overlay Off"
                                selected: settings.performanceOverlay
                                implicitHeight: 38
                                onClicked: settings.performanceOverlay = !settings.performanceOverlay
                            }

                            Text { text: "Ambient Idle Screensaver"; color: "#E2EEFF"; font.pixelSize: 14 }
                            Row {
                                spacing: 10
                                FocusButton {
                                    text: settings.screensaverEnabled ? "Enabled" : "Disabled"
                                    selected: settings.screensaverEnabled
                                    implicitHeight: 36
                                    onClicked: settings.screensaverEnabled = !settings.screensaverEnabled
                                }
                                Text {
                                    text: settings.screensaverTimeoutMinutes + " min idle"
                                    color: "#B8CCE8"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Slider {
                                    width: 140
                                    from: 1; to: 30; stepSize: 1; value: settings.screensaverTimeoutMinutes
                                    onMoved: settings.screensaverTimeoutMinutes = Math.round(value)
                                }
                            }

                            Text { text: "Launch Automatically with Windows 11"; color: "#E2EEFF"; font.pixelSize: 14 }
                            FocusButton {
                                text: settings.startupWithWindows ? "Enabled on Login" : "Disabled"
                                selected: settings.startupWithWindows
                                implicitHeight: 38
                                onClicked: settings.startupWithWindows = !settings.startupWithWindows
                            }

                            Text { text: "SteamGridDB API Key (Optional Online Cover Art)"; color: "#E2EEFF"; font.pixelSize: 14 }
                            TextField {
                                Layout.fillWidth: true
                                text: settings.steamGridDbApiKey
                                echoMode: TextInput.Password
                                placeholderText: "Optional free key from steamgriddb.com/profile/preferences/api"
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                                onEditingFinished: settings.steamGridDbApiKey = text
                            }
                        }
                    }

                    // TAB 5: UPDATES & ROLLBACK
                    ColumnLayout {
                        visible: settingsView.activeTab === 5
                        Layout.fillWidth: true
                        spacing: 16

                        Text { text: "Automatic Updates, Backups & Rollback"; color: "#FFFFFF"; font.family: settings.fontFamily; font.pixelSize: 24; font.bold: true }

                        GlassPanel {
                            Layout.fillWidth: true
                            implicitHeight: updCol.implicitHeight + 36
                            glassRadius: 18
                            opacityBoost: 0.12

                            ColumnLayout {
                                id: updCol
                                anchors.fill: parent
                                anchors.margins: 18
                                spacing: 10

                                Text {
                                    text: "Installed Version: v" + updater.currentVersion + "   •   " + updater.statusMessage
                                    color: "#FFFFFF"
                                    font.family: settings.fontFamily
                                    font.pixelSize: 14
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }

                                ProgressBar {
                                    visible: updater.downloading
                                    Layout.fillWidth: true
                                    value: updater.downloadProgress
                                }

                                Text {
                                    visible: updater.updateAvailable && (updater.availableManifest.releaseNotes || "").length > 0
                                    text: "Release Notes (v" + (updater.availableManifest.version || "") + "):\n" + (updater.availableManifest.releaseNotes || "")
                                    color: "#C4DAF8"
                                    font.pixelSize: 13
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }

                                Row {
                                    spacing: 12
                                    FocusButton {
                                        text: updater.checking ? "Checking..." : "Check for Updates"
                                        enabledButton: !updater.checking && !updater.downloading
                                        onClicked: updater.checkForUpdates()
                                    }
                                    FocusButton {
                                        visible: updater.updateAvailable
                                        text: "Download & Install v" + (updater.availableManifest.version || "")
                                        primary: true
                                        enabledButton: !updater.downloading
                                        onClicked: {
                                            library.createBackupSnapshot()
                                            updater.downloadAndInstallUpdate()
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Text { text: "Update Manifest Feed URL (GitHub Releases or Local Dev Companion)"; color: "#B8CCE8"; font.pixelSize: 12 }
                            TextField {
                                Layout.fillWidth: true
                                text: settings.updateManifestUrl
                                placeholderText: "e.g. https://github.com/<user>/<repo>/releases/latest/download/manifest.json  or  http://127.0.0.1:47821/manifest.json"
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                                onEditingFinished: settings.updateManifestUrl = text
                            }
                        }

                        Row {
                            spacing: 12
                            FocusButton {
                                text: settings.autoCheckUpdates ? "Auto-Check Updates: ON" : "Auto-Check Updates: OFF"
                                selected: settings.autoCheckUpdates
                                onClicked: settings.autoCheckUpdates = !settings.autoCheckUpdates
                            }
                            FocusButton {
                                text: settings.autoBackupOnLaunch ? "Auto-Backup Library: ON" : "Auto-Backup Library: OFF"
                                selected: settings.autoBackupOnLaunch
                                onClicked: settings.autoBackupOnLaunch = !settings.autoBackupOnLaunch
                            }
                        }

                        Text { text: "Previous Installation Backups (One-Click Rollback)"; color: "#B8CCE8"; font.pixelSize: 13; font.bold: true }

                        Text {
                            visible: updater.installBackups.length === 0
                            text: "No previous binary backups yet. A backup of the current installation is created automatically before every update."
                            color: Qt.rgba(0.80, 0.88, 1.0, 0.65)
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: updater.installBackups
                            GlassPanel {
                                Layout.fillWidth: true
                                implicitHeight: 46
                                glassRadius: 12
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 14
                                    anchors.rightMargin: 10
                                    Text { text: modelData.name + "  (" + modelData.date + ")"; color: "#FFFFFF"; font.pixelSize: 13; Layout.fillWidth: true }
                                    FocusButton {
                                        text: "Rollback"
                                        danger: true
                                        implicitWidth: 96
                                        implicitHeight: 32
                                        fontSize: 12
                                        onClicked: updater.rollbackToInstallBackup(modelData.path)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
