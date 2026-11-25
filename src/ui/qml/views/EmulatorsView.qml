import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Item {
    id: emuView

    property string activeBrowseEmulatorId: ""
    property string activeFolderEmulatorId: ""
    property bool showAddForm: false

    signal backRequested()

    FileDialog {
        id: exePicker
        title: "Select Emulator Executable"
        onAccepted: {
            var p = library.normalizeFileUrl(selectedFile.toString())
            if (emuView.activeBrowseEmulatorId === "__new__") {
                newExeField.text = p
            } else if (emuView.activeBrowseEmulatorId.length > 0) {
                emulators.updateEmulator(emuView.activeBrowseEmulatorId, { executablePath: p })
            }
        }
    }

    FolderDialog {
        id: romFolderPicker
        title: "Select Game / ROM Directory"
        onAccepted: {
            var dir = library.normalizeFileUrl(selectedFolder.toString())
            if (emuView.activeFolderEmulatorId.length > 0) {
                emulators.addGameDirectory(emuView.activeFolderEmulatorId, dir)
            }
        }
    }

    Flickable {
        anchors.fill: parent
        anchors.leftMargin: 40
        anchors.rightMargin: 40
        anchors.topMargin: 16
        anchors.bottomMargin: 24
        contentHeight: contentCol.implicitHeight + 48
        clip: true

        ColumnLayout {
            id: contentCol
            width: parent.width
            spacing: 20

            // Header
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                FocusButton {
                    text: "← Back"
                    onClicked: {
                        audio.playBack()
                        emuView.backRequested()
                    }
                }

                Column {
                    spacing: 2
                    Text {
                        text: "Emulator Integrations"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 26
                        font.weight: Font.Bold
                    }
                    Text {
                        text: "Configure Lichii (Nintendo Switch 2) and other emulators. No ROMs, firmware, or keys are bundled."
                        color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                        font.family: settings.fontFamily
                        font.pixelSize: 13
                    }
                }

                Item { Layout.fillWidth: true }

                FocusButton {
                    text: "Scan All Emulator Folders"
                    onClicked: library.scanAllConfiguredDirectories()
                }
                FocusButton {
                    text: showAddForm ? "Cancel" : "+ Add Emulator"
                    primary: !showAddForm
                    onClicked: showAddForm = !showAddForm
                }
            }

            // Add New Emulator Panel
            GlassPanel {
                visible: emuView.showAddForm
                Layout.fillWidth: true
                implicitHeight: addCol.implicitHeight + 44
                glassRadius: 24
                elevated: true
                opacityBoost: 0.14

                ColumnLayout {
                    id: addCol
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 12

                    Text {
                        text: "Add Emulator Profile"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                    }

                    // Quick templates
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: emulators.presetTemplates()
                            FocusButton {
                                text: modelData.name + " (" + modelData.platform + ")"
                                implicitHeight: 32
                                buttonRadius: 16
                                fontSize: 12
                                onClicked: {
                                    newNameField.text = modelData.name
                                    newPlatField.text = modelData.platform
                                    newArgsField.text = modelData.launchArguments
                                    newExtsField.text = modelData.supportedExtensions.join(", ")
                                }
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 14
                        rowSpacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Emulator Name"; color: "#BDD0EB"; font.pixelSize: 12 }
                            TextField {
                                id: newNameField
                                Layout.fillWidth: true
                                placeholderText: "e.g. Dolphin, PCSX2, RetroArch"
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Platform"; color: "#BDD0EB"; font.pixelSize: 12 }
                            TextField {
                                id: newPlatField
                                Layout.fillWidth: true
                                placeholderText: "e.g. GameCube / Wii"
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            Text { text: "Emulator Executable Path"; color: "#BDD0EB"; font.pixelSize: 12 }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                TextField {
                                    id: newExeField
                                    Layout.fillWidth: true
                                    placeholderText: "Browse to emulator .exe"
                                    color: "#FFFFFF"
                                    background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                                }
                                FocusButton {
                                    text: "Browse..."
                                    implicitHeight: 38
                                    onClicked: {
                                        emuView.activeBrowseEmulatorId = "__new__"
                                        exePicker.open()
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Launch Arguments Template"; color: "#BDD0EB"; font.pixelSize: 12 }
                            TextField {
                                id: newArgsField
                                Layout.fillWidth: true
                                text: "\"{rom}\""
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Supported Extensions (comma-separated)"; color: "#BDD0EB"; font.pixelSize: 12 }
                            TextField {
                                id: newExtsField
                                Layout.fillWidth: true
                                placeholderText: ".iso, .chd, .rvz"
                                color: "#FFFFFF"
                                background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                            }
                        }
                    }

                    FocusButton {
                        text: "Save Emulator Profile"
                        primary: true
                        onClicked: {
                            var exts = newExtsField.text.split(/[,;\s]+/).filter(function(s){ return s.length > 0 })
                            var id = emulators.addEmulator({
                                name: newNameField.text,
                                platform: newPlatField.text,
                                executablePath: newExeField.text,
                                launchArguments: newArgsField.text,
                                supportedExtensions: exts
                            })
                            if (id.length > 0) {
                                newNameField.text = ""
                                newPlatField.text = ""
                                newExeField.text = ""
                                newArgsField.text = "\"{rom}\""
                                newExtsField.text = ""
                                emuView.showAddForm = false
                            }
                        }
                    }
                }
            }

            // Emulator Cards (Lichii always appears first as dedicated Switch 2 integration)
            Repeater {
                model: emulators.emulators

                GlassPanel {
                    id: emuCard
                    Layout.fillWidth: true
                    implicitHeight: cardCol.implicitHeight + 44
                    glassRadius: 24
                    elevated: !!modelData.isDedicatedLichii
                    accentColor: modelData.isDedicatedLichii ? "#FF4D6D" : settings.accentColor
                    opacityBoost: modelData.isDedicatedLichii ? 0.14 : 0.08

                    ColumnLayout {
                        id: cardCol
                        anchors.fill: parent
                        anchors.margins: 22
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Column {
                                spacing: 3
                                Row {
                                    spacing: 10
                                    Text {
                                        text: modelData.name
                                        color: "#FFFFFF"
                                        font.family: settings.fontFamily
                                        font.pixelSize: 22
                                        font.weight: Font.Bold
                                    }
                                    Rectangle {
                                        visible: !!modelData.isDedicatedLichii
                                        height: 22
                                        width: dedLbl.implicitWidth + 16
                                        radius: 11
                                        color: Qt.rgba(1.0, 0.30, 0.43, 0.28)
                                        border.width: 1
                                        border.color: "#FF4D6D"
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            id: dedLbl
                                            anchors.centerIn: parent
                                            text: "DEDICATED SWITCH 2 INTEGRATION"
                                            color: "#FFD5DE"
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                    Rectangle {
                                        height: 22
                                        width: statLbl.implicitWidth + 16
                                        radius: 11
                                        color: modelData.executableExists ? Qt.rgba(0.16, 0.85, 0.55, 0.25) : Qt.rgba(1.0, 0.72, 0.2, 0.22)
                                        border.width: 1
                                        border.color: modelData.executableExists ? "#34D399" : "#FBBF24"
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            id: statLbl
                                            anchors.centerIn: parent
                                            text: modelData.executableExists ? "Executable Ready" : "Set Executable Path"
                                            color: "#FFFFFF"
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                }

                                Text {
                                    text: "Platform: " + modelData.platform
                                    color: Qt.rgba(0.84, 0.90, 1.0, 0.76)
                                    font.family: settings.fontFamily
                                    font.pixelSize: 13
                                }
                            }

                            Item { Layout.fillWidth: true }

                            FocusButton {
                                text: "Scan Folders"
                                implicitHeight: 38
                                onClicked: library.scanEmulatorDirectories(modelData.id)
                            }
                            FocusButton {
                                visible: !modelData.isDedicatedLichii
                                text: "Remove"
                                danger: true
                                implicitHeight: 38
                                onClicked: emulators.removeEmulator(modelData.id)
                            }
                        }

                        Text {
                            visible: (modelData.notes || "").length > 0
                            Layout.fillWidth: true
                            text: modelData.notes || ""
                            color: Qt.rgba(0.84, 0.90, 1.0, 0.74)
                            font.family: settings.fontFamily
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }

                        // Editable Executable, Launch Args & Extensions
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 14
                            rowSpacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.columnSpan: 2
                                Text { text: "Executable Path"; color: "#BDD0EB"; font.pixelSize: 12 }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    TextField {
                                        id: emuExeInput
                                        Layout.fillWidth: true
                                        text: modelData.executablePath || ""
                                        placeholderText: "Select local emulator executable (e.g. C:\\Emulators\\Lichii\\lichii.exe)"
                                        color: "#FFFFFF"
                                        background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                                        onEditingFinished: emulators.updateEmulator(modelData.id, { executablePath: text })
                                    }
                                    FocusButton {
                                        text: "Browse..."
                                        implicitHeight: 38
                                        onClicked: {
                                            emuView.activeBrowseEmulatorId = modelData.id
                                            exePicker.open()
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: "Launch Arguments ({rom}, {rom_dir}, {rom_name}, {title})"; color: "#BDD0EB"; font.pixelSize: 12 }
                                TextField {
                                    id: emuArgsInput
                                    Layout.fillWidth: true
                                    text: modelData.launchArguments || "\"{rom}\""
                                    color: "#FFFFFF"
                                    background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                                    onEditingFinished: emulators.updateEmulator(modelData.id, { launchArguments: text })
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: "Associated File Extensions"; color: "#BDD0EB"; font.pixelSize: 12 }
                                TextField {
                                    id: emuExtsInput
                                    Layout.fillWidth: true
                                    text: (modelData.supportedExtensions || []).join(", ")
                                    color: "#FFFFFF"
                                    background: GlassPanel { glassRadius: 10; opacityBoost: 0.1 }
                                    onEditingFinished: emulators.updateEmulator(modelData.id, { supportedExtensions: text })
                                }
                            }
                        }

                        // Configured Game Directories
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "Game Directories (" + (modelData.gameDirectories ? modelData.gameDirectories.length : 0) + ")"
                                    color: "#BDD0EB"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                Item { Layout.fillWidth: true }
                                FocusButton {
                                    text: "+ Add Game Folder"
                                    implicitHeight: 32
                                    fontSize: 12
                                    onClicked: {
                                        emuView.activeFolderEmulatorId = modelData.id
                                        romFolderPicker.open()
                                    }
                                }
                            }

                            Repeater {
                                model: modelData.gameDirectories || []
                                GlassPanel {
                                    Layout.fillWidth: true
                                    implicitHeight: 38
                                    glassRadius: 10
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 8
                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData
                                            color: "#E4EEFF"
                                            font.pixelSize: 12
                                            elide: Text.ElideMiddle
                                        }
                                        FocusButton {
                                            text: "Remove"
                                            danger: true
                                            implicitWidth: 78
                                            implicitHeight: 28
                                            fontSize: 11
                                            onClicked: emulators.removeGameDirectory(emuCard.emuId, modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    property string emuId: modelData.id
                }
            }
        }
    }
}
