#pragma once

#include "core/SettingsManager.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

struct SDL_Gamepad;

namespace Aether {

class ControllerManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool connected READ isConnected NOTIFY controllerStateChanged)
    Q_PROPERTY(int connectedCount READ connectedCount NOTIFY controllerStateChanged)
    Q_PROPERTY(QString primaryName READ primaryName NOTIFY controllerStateChanged)
    Q_PROPERTY(QString controllerFamily READ controllerFamily NOTIFY controllerStateChanged) // xbox, playstation, nintendo, generic
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY controllerStateChanged)         // -1 if wired/unknown
    Q_PROPERTY(QString batteryState READ batteryState NOTIFY controllerStateChanged)         // battery, charging, charged, wired, unknown
    Q_PROPERTY(QString inputMode READ inputMode NOTIFY inputModeChanged)                     // controller or keyboard
    Q_PROPERTY(bool inputSuspended READ inputSuspended WRITE setInputSuspended NOTIFY inputSuspendedChanged)
    Q_PROPERTY(QString remappingAction READ remappingAction NOTIFY remappingChanged)
    Q_PROPERTY(QVariantList connectedControllers READ connectedControllersVariant NOTIFY controllerStateChanged)

public:
    explicit ControllerManager(SettingsManager &settings, QObject *parent = nullptr);
    ~ControllerManager() override;

    bool initialize();
    void shutdown();

    bool isConnected() const { return !m_gamepads.isEmpty(); }
    int connectedCount() const { return m_gamepads.size(); }
    QString primaryName() const { return m_primaryName; }
    QString controllerFamily() const { return m_controllerFamily; }
    int batteryPercent() const { return m_batteryPercent; }
    QString batteryState() const { return m_batteryState; }
    QString inputMode() const { return m_inputMode; }
    bool inputSuspended() const { return m_inputSuspended; }
    void setInputSuspended(bool suspended);
    QString remappingAction() const { return m_remappingAction; }
    QVariantList connectedControllersVariant() const;

    Q_INVOKABLE void notifyKeyboardMouseActivity();
    Q_INVOKABLE void rumble(double lowFreq = 0.25, double highFreq = 0.35, int durationMs = 60);
    Q_INVOKABLE void testRumble();
    Q_INVOKABLE void beginRemap(const QString &actionName);
    Q_INVOKABLE void cancelRemap();
    Q_INVOKABLE QString glyphLabelForAction(const QString &actionName) const;

signals:
    void controllerStateChanged();
    void inputModeChanged();
    void inputSuspendedChanged();
    void remappingChanged();
    void userActivity();

    // Navigation & semantic action signals handled by QML
    void navigate(const QString &direction); // up, down, left, right
    void actionTriggered(const QString &action); // confirm, back, info, options, prevCategory, nextCategory, prevCollection, nextCollection, quickMenu
    void controllerConnected(const QString &name);
    void controllerDisconnected(const QString &name);

private slots:
    void pollEvents();
    void refreshBatteryStatus();

private:
    void openGamepad(uint32_t instanceId);
    void closeGamepad(uint32_t instanceId);
    void updatePrimaryInfo();
    void setInputMode(const QString &mode);
    void handlePhysicalButton(const QString &buttonId);
    void processDirectionalHold(const QString &direction);

    SettingsManager &m_settings;
    bool m_sdlInitialized = false;
    bool m_inputSuspended = false;
    QTimer m_pollTimer;
    QTimer m_batteryTimer;

    QHash<uint32_t, SDL_Gamepad *> m_gamepads;
    uint32_t m_primaryId = 0;
    QString m_primaryName;
    QString m_controllerFamily = QStringLiteral("xbox");
    int m_batteryPercent = -1;
    QString m_batteryState = QStringLiteral("unknown");
    QString m_inputMode = QStringLiteral("keyboard");
    QString m_remappingAction;

    // Trigger edge detection
    bool m_leftTriggerDown = false;
    bool m_rightTriggerDown = false;

    // Stick/D-pad hold repeat
    QString m_heldDirection;
    QElapsedTimer m_holdTimer;
    bool m_holdRepeatStarted = false;
    int m_dpadMask = 0;
    float m_stickX = 0.0f;
    float m_stickY = 0.0f;
};

} // namespace Aether
