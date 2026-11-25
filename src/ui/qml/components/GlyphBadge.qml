import QtQuick

Row {
    id: root
    property string actionName: "confirm"
    property string label: ""

    spacing: 8

    readonly property string glyphText: {
        // Reactively re-evaluate when inputMode, controllerFamily, or settings change
        var _m = controller.inputMode
        var _f = controller.controllerFamily
        var _b = settings.buttonMappings
        return controller.glyphLabelForAction(actionName)
    }

    Rectangle {
        height: 24
        width: Math.max(26, keyLabel.implicitWidth + 14)
        radius: controller.inputMode === "controller" ? 12 : 7
        color: Qt.rgba(1, 1, 1, 0.14)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.28)
        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: keyLabel
            anchors.centerIn: parent
            text: root.glyphText
            color: "#FFFFFF"
            font.family: settings.fontFamily
            font.pixelSize: 11
            font.bold: true
        }
    }

    Text {
        visible: root.label.length > 0
        text: root.label
        color: Qt.rgba(0.90, 0.94, 1.0, 0.85)
        font.family: settings.fontFamily
        font.pixelSize: 13
        font.weight: Font.Medium
        anchors.verticalCenter: parent.verticalCenter
    }
}
