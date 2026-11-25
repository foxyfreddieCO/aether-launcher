import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Item {
    id: libMgr

    signal backRequested()
    signal openGameDetails(string gameId)

    FileDialog {
        id: exeDialog
        title: "Add Windows Executable / Application"
        nameFilters: ["Executables (*.exe *.bat *.cmd *.lnk *.app *)", "All Files (*)"]
        onAccepted: library.addManualExecutable(selectedFile.toString())
    }

    FileDialog {
        id: romDialog
        title: "Add Game File / ROM"
        onAccepted: {
            var emuId = ""
            if (romEmuSelect.currentIndex > 0) {
                var list = emulators.emulators
                if (romEmuSelect.currentIndex - 1 < list.length) {
                    emuId = list[romEmuSelect.currentIndex - 1].id
                }
            }
            library.addGameFile(selectedFile.toString(), emuId)
        }
    }

    FolderDialog {
        id: scanDialog
        title: "Select Directory to Scan for Games & ROMs"
        onAccepted: library.scanDirectory(selectedFolder.toString())
    }

    FileDialog {
        id: exportDialog
        title: "Export Launcher Configuration Bundle"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["Aether Bundle (*.json)"]
        onAccepted: library.exportConfigurationBundle(selectedFile.toString())
    }

    FileDialog {
        id: importDialog
        title: "Import Launcher Configuration Bundle"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Aether Bundle (*.json)"]
        onAccepted: library.importConfigurationBundle(selectedFile.toString())
    }

    Flickable {
        anchors.fill: parent
        anchors.leftMargin: 40
        anchors.rightMargin: 40
        anchors.topMargin: 16
        anchors.bottomMargin: 24
        contentHeight: col.implicitHeight + 48
        clip: true

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                FocusButton {
                    text: "← Back"
                    onClicked: {
                        audio.playBack()
                        libMgr.backRequested()
                    }
                }

                Column {
                    spacing: 2
                    Text {
                        text: "Library & Collections Manager"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 26
                        font.weight: Font.Bold
                    }
                    Text {
                        text: library.totalGamesCount + " games  •  " + library.formatPlaytime(library.totalPlaytimeSeconds) + " total playtime"
                        color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                        font.family: settings.fontFamily
                        font.pixelSize: 13
                    }
                }

                Item { Layout.fillWidth: true }
            }

            // Add Games Card
            GlassPanel {
                Layout.fillWidth: true
                implicitHeight: addGamesCol.implicitHeight + 44
                glassRadius: 24
                opacityBoost: 0.10

                ColumnLayout {
                    id: addGamesCol
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 14

                    Text {
                        text: "Add Games & Scan Directories"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        FocusButton {
                            text: "+ Add Windows .exe Game"
                            primary: true
                            onClicked: exeDialog.open()
                        }

                        ComboBox {
                            id: romEmuSelect
                            Layout.preferredWidth: 230
                            implicitHeight: 44
                            model: {
                                var list = ["Auto-detect Emulator by Extension"]
                                var emus = emulators.emulators
                                for (var i = 0; i < emus.length; ++i) {
                                    list.push(emus[i].name + " (" + emus[i].platform + ")")
                                }
                                return list
                            }
                            background: GlassPanel { glassRadius: 14; opacityBoost: 0.08 }
                            contentItem: Text {
                                leftPadding: 12
                                text: romEmuSelect.displayText
                                color: "#FFFFFF"
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        FocusButton {
                            text: "+ Add ROM / Game File"
                            onClicked: romDialog.open()
                        }

                        FocusButton {
                            text: "Scan Directory..."
                            onClicked: scanDialog.open()
                        }
                    }
                }
            }

            // Collections Manager Card
            GlassPanel {
                Layout.fillWidth: true
                implicitHeight: colMgrLayout.implicitHeight + 44
                glassRadius: 24
                opacityBoost: 0.10

                ColumnLayout {
                    id: colMgrLayout
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 12

                    Text {
                        text: "Custom Collections & Playlists"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        TextField {
                            id: newColName
                            Layout.preferredWidth: 260
                            placeholderText: "New collection name (e.g. Co-op Night)"
                            color: "#FFFFFF"
                            background: GlassPanel { glassRadius: 12; opacityBoost: 0.1 }
                        }
                        TextField {
                            id: newColDesc
                            Layout.fillWidth: true
                            placeholderText: "Optional description"
                            color: "#FFFFFF"
                            background: GlassPanel { glassRadius: 12; opacityBoost: 0.1 }
                        }
                        FocusButton {
                            text: "+ Create Collection"
                            primary: true
                            onClicked: {
                                if (newColName.text.trim().length > 0) {
                                    library.createCollection(newColName.text, newColDesc.text, settings.accentColor)
                                    newColName.text = ""
                                    newColDesc.text = ""
                                }
                            }
                        }
                    }

                    Repeater {
                        model: library.collections
                        GlassPanel {
                            Layout.fillWidth: true
                            implicitHeight: 46
                            glassRadius: 12
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 10
                                Text {
                                    text: modelData.name + "  (" + modelData.gameCount + " games)"
                                    color: "#FFFFFF"
                                    font.family: settings.fontFamily
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.description || ""
                                    color: Qt.rgba(0.84, 0.90, 1.0, 0.65)
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                                FocusButton {
                                    text: "Delete"
                                    danger: true
                                    implicitWidth: 80
                                    implicitHeight: 32
                                    fontSize: 12
                                    onClicked: library.removeCollection(modelData.id)
                                }
                            }
                        }
                    }
                }
            }

            // Backups & Import/Export Card
            GlassPanel {
                Layout.fillWidth: true
                implicitHeight: bkCol.implicitHeight + 44
                glassRadius: 24
                opacityBoost: 0.10

                ColumnLayout {
                    id: bkCol
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 12

                    Text {
                        text: "Library Backups & Configuration Portability"
                        color: "#FFFFFF"
                        font.family: settings.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "All game libraries, artwork, emulator settings, and custom themes are stored strictly in your user data directory — never inside the installation folder."
                        color: Qt.rgba(0.84, 0.90, 1.0, 0.72)
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 12
                        FocusButton {
                            text: "Create Backup Snapshot Now"
                            primary: true
                            onClicked: {
                                library.createBackupSnapshot()
                                backupList.model = library.listBackups()
                            }
                        }
                        FocusButton {
                            text: "Export Configuration (.json)"
                            onClicked: exportDialog.open()
                        }
                        FocusButton {
                            text: "Import Configuration (.json)"
                            onClicked: importDialog.open()
                        }
                    }

                    Repeater {
                        id: backupList
                        model: library.listBackups()
                        GlassPanel {
                            Layout.fillWidth: true
                            implicitHeight: 38
                            glassRadius: 10
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                Text { text: modelData.name; color: "#E6F0FF"; font.pixelSize: 12; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: modelData.modified; color: Qt.rgba(0.82, 0.89, 1.0, 0.65); font.pixelSize: 12 }
                            }
                        }
                    }
                }
            }
        }
    }
}
