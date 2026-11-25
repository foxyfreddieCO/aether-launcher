import QtQuick
import QtQuick.Effects

Item {
    id: root
    property string imageSource: ""
    property color accentColor: settings.accentColor
    property color secondaryAccent: settings.secondaryAccent
    property color surfaceColor: settings.surfaceColor
    property real blurIntensity: settings.blurIntensity
    property bool animationsEnabled: settings.animationsEnabled
    property real animationSpeed: settings.animationSpeed

    property bool _useLayerA: true

    onImageSourceChanged: {
        if (_useLayerA) {
            imgB.source = imageSource
            _useLayerA = false
        } else {
            imgA.source = imageSource
            _useLayerA = true
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter(root.surfaceColor, 1.35) }
            GradientStop { position: 0.55; color: root.surfaceColor }
            GradientStop { position: 1.0; color: Qt.darker(root.surfaceColor, 1.65) }
        }
    }

    // Subtle atmospheric liquid light orbs
    Rectangle {
        id: orbPrimary
        width: Math.max(root.width * 0.58, 620)
        height: width
        radius: width / 2
        x: -width * 0.18
        y: -height * 0.25
        opacity: 0.28
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.accentColor }
            GradientStop { position: 1.0; color: "transparent" }
        }

        SequentialAnimation on x {
            running: root.animationsEnabled
            loops: Animation.Infinite
            NumberAnimation {
                to: -orbPrimary.width * 0.06
                duration: Math.round(11000 / root.animationSpeed)
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                to: -orbPrimary.width * 0.22
                duration: Math.round(11000 / root.animationSpeed)
                easing.type: Easing.InOutSine
            }
        }
    }

    Rectangle {
        id: orbSecondary
        width: Math.max(root.width * 0.52, 540)
        height: width
        radius: width / 2
        x: root.width - width * 0.72
        y: root.height - height * 0.65
        opacity: 0.24
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.secondaryAccent }
            GradientStop { position: 1.0; color: "transparent" }
        }

        SequentialAnimation on y {
            running: root.animationsEnabled
            loops: Animation.Infinite
            NumberAnimation {
                to: root.height - orbSecondary.height * 0.52
                duration: Math.round(13500 / root.animationSpeed)
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                to: root.height - orbSecondary.height * 0.70
                duration: Math.round(13500 / root.animationSpeed)
                easing.type: Easing.InOutSine
            }
        }
    }

    // Crossfading dynamic game artwork / static wallpaper
    Item {
        id: backdropContainer
        anchors.fill: parent
        visible: imgA.status === Image.Ready || imgB.status === Image.Ready

        Image {
            id: imgA
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
            opacity: root._useLayerA && status === Image.Ready ? 0.62 : 0.0
            Behavior on opacity {
                enabled: root.animationsEnabled
                NumberAnimation { duration: Math.round(450 / root.animationSpeed); easing.type: Easing.OutCubic }
            }
        }

        Image {
            id: imgB
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
            opacity: !root._useLayerA && status === Image.Ready ? 0.62 : 0.0
            Behavior on opacity {
                enabled: root.animationsEnabled
                NumberAnimation { duration: Math.round(450 / root.animationSpeed); easing.type: Easing.OutCubic }
            }
        }

        layer.enabled: root.blurIntensity > 0.04
        layer.effect: MultiEffect {
            blurEnabled: true
            blurMax: 64
            blur: 0.55 * root.blurIntensity
            saturation: 0.15
        }
    }

    // Cinema vignette for crisp legibility over any artwork
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.03, 0.05, 0.09, 0.55) }
            GradientStop { position: 0.45; color: Qt.rgba(0.03, 0.05, 0.09, 0.35) }
            GradientStop { position: 1.0; color: Qt.rgba(0.02, 0.03, 0.06, 0.88) }
        }
    }
}
