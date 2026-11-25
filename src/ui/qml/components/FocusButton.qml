import QtQuick

Item {
    id: btn

    property string text: ""
    property string iconText: ""
    property bool primary: false
    property bool danger: false
    property bool highlighted: false
    property bool selected: false
    property bool enabledButton: true
    property real buttonRadius: 14
    property int fontSize: 14

    signal clicked()

    implicitWidth: Math.max(112, contentRow.implicitWidth + 36)
    implicitHeight: 44
    opacity: enabledButton ? 1.0 : 0.45

    readonly property bool isFocused: highlighted || mouseArea.containsMouse || btn.activeFocus

    scale: mouseArea.pressed ? 0.96 : (isFocused ? 1.03 : 1.0)
    Behavior on scale {
        enabled: settings.animationsEnabled
        NumberAnimation { duration: Math.round(130 / settings.animationSpeed); easing.type: Easing.OutCubic }
    }

    GlassPanel {
        anchors.fill: parent
        glassRadius: btn.buttonRadius
        focused: btn.isFocused
        elevated: btn.primary || btn.selected
        accentColor: btn.danger ? "#FF5C7A" : settings.accentColor
        tintColor: btn.danger
            ? Qt.rgba(0.55, 0.10, 0.18, 1.0)
            : (btn.primary
                ? Qt.darker(settings.accentColor, 1.6)
                : (btn.selected ? Qt.darker(settings.accentColor, 2.2) : Qt.rgba(0.08, 0.12, 0.20, 1.0)))
        opacityBoost: btn.primary ? 0.42 : (btn.selected ? 0.24 : 0.04)
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 8

        Text {
            visible: btn.iconText.length > 0
            text: btn.iconText
            color: btn.primary ? "#FFFFFF" : (btn.isFocused ? "#FFFFFF" : "#D6E2F5")
            font.family: settings.fontFamily
            font.pixelSize: btn.fontSize + 1
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: btn.text.length > 0
            text: btn.text
            color: btn.primary ? "#FFFFFF" : (btn.isFocused ? "#FFFFFF" : "#E6EEF8")
            font.family: settings.fontFamily
            font.pixelSize: btn.fontSize
            font.weight: btn.primary || btn.isFocused ? Font.DemiBold : Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        enabled: btn.enabledButton
        cursorShape: Qt.PointingHandCursor
        onEntered: {
            controller.notifyKeyboardMouseActivity()
            audio.playNavigate()
        }
        onClicked: {
            controller.notifyKeyboardMouseActivity()
            audio.playConfirm()
            btn.clicked()
        }
    }
}
