import QtQuick

Rectangle {
    id: panel

    property real glassRadius: 20
    property bool focused: false
    property bool elevated: false
    property color tintColor: Qt.rgba(0.08, 0.11, 0.18, 1.0)
    property color accentColor: settings.accentColor
    property real opacityBoost: 0.0

    radius: glassRadius
    color: Qt.rgba(
        tintColor.r,
        tintColor.g,
        tintColor.b,
        Math.min(0.92, Math.max(0.08, settings.glassOpacity + opacityBoost + (focused ? 0.12 : 0.0)))
    )
    border.width: focused ? 2 : 1
    border.color: focused
        ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.92)
        : Qt.rgba(1, 1, 1, elevated ? 0.22 : 0.14)

    Behavior on border.color {
        enabled: settings.animationsEnabled
        ColorAnimation { duration: Math.round(160 / settings.animationSpeed) }
    }

    // Subtle liquid glass diagonal sheen reflection
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, panel.radius - 1)
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, panel.focused ? 0.16 : 0.10) }
            GradientStop { position: 0.32; color: Qt.rgba(1, 1, 1, 0.03) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.14) }
        }
        z: -1
    }

    // Top specular glass rim highlight
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: panel.radius * 0.65
        anchors.rightMargin: panel.radius * 0.65
        anchors.topMargin: 1
        height: 1
        color: Qt.rgba(1, 1, 1, panel.focused ? 0.55 : 0.28)
        z: 10
    }

    // Outer focus halo for unmistakable controller focus indication
    Rectangle {
        anchors.fill: parent
        anchors.margins: -5
        radius: panel.radius + 5
        color: "transparent"
        border.width: 3
        border.color: Qt.rgba(panel.accentColor.r, panel.accentColor.g, panel.accentColor.b, 0.36)
        visible: panel.focused
        z: -2
    }
}
