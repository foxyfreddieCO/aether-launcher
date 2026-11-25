#pragma once

#include "core/Models.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QWindow>

namespace Aether {

class SystemIntegration : public QObject {
    Q_OBJECT

    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY telemetryChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY telemetryChanged)
    Q_PROPERTY(bool gpuAvailable READ gpuAvailable NOTIFY telemetryChanged)
    Q_PROPERTY(double ramUsage READ ramUsage NOTIFY telemetryChanged)
    Q_PROPERTY(double ramUsedGb READ ramUsedGb NOTIFY telemetryChanged)
    Q_PROPERTY(double ramTotalGb READ ramTotalGb NOTIFY telemetryChanged)
    Q_PROPERTY(double temperatureC READ temperatureC NOTIFY telemetryChanged)
    Q_PROPERTY(bool temperatureAvailable READ temperatureAvailable NOTIFY telemetryChanged)

    Q_PROPERTY(int systemVolume READ systemVolume WRITE setSystemVolume NOTIFY audioStateChanged)
    Q_PROPERTY(bool systemMuted READ systemMuted WRITE setSystemMuted NOTIFY audioStateChanged)
    Q_PROPERTY(QVariantList audioOutputDevices READ audioOutputDevicesVariant NOTIFY audioDevicesChanged)
    Q_PROPERTY(QString currentAudioDeviceName READ currentAudioDeviceName NOTIFY audioDevicesChanged)

    Q_PROPERTY(QString currentTime READ currentTime NOTIFY clockChanged)
    Q_PROPERTY(QString currentDate READ currentDate NOTIFY clockChanged)
    Q_PROPERTY(QVariantList monitors READ monitorsVariant NOTIFY monitorsChanged)
    Q_PROPERTY(QString platformName READ platformName CONSTANT)

public:
    explicit SystemIntegration(QObject *parent = nullptr);
    ~SystemIntegration() override;

    void startMonitoring();
    void stopMonitoring();

    double cpuUsage() const { return m_telemetry.cpuUsagePercent; }
    double gpuUsage() const { return m_telemetry.gpuUsagePercent; }
    bool gpuAvailable() const { return m_telemetry.gpuAvailable; }
    double ramUsage() const { return m_telemetry.ramUsagePercent; }
    double ramUsedGb() const { return m_telemetry.ramUsedGb; }
    double ramTotalGb() const { return m_telemetry.ramTotalGb; }
    double temperatureC() const { return m_telemetry.temperatureCelsius; }
    bool temperatureAvailable() const { return m_telemetry.temperatureAvailable; }

    int systemVolume() const { return m_volumePercent; }
    void setSystemVolume(int percent);
    bool systemMuted() const { return m_muted; }
    void setSystemMuted(bool muted);

    QVariantList audioOutputDevicesVariant() const;
    QString currentAudioDeviceName() const;
    QString currentTime() const { return m_currentTime; }
    QString currentDate() const { return m_currentDate; }
    QVariantList monitorsVariant() const;
    QString platformName() const;

    Q_INVOKABLE void refreshAudioDevices();
    Q_INVOKABLE bool selectAudioOutputDevice(const QString &deviceId);
    Q_INVOKABLE bool openBluetoothSettings();
    Q_INVOKABLE bool openWifiSettings();
    Q_INVOKABLE bool openSoundSettings();
    Q_INVOKABLE bool setStartupWithWindows(bool enabled);
    Q_INVOKABLE void applyWindowStyle(QWindow *window);
    Q_INVOKABLE void moveWindowToMonitor(QWindow *window, int monitorIndex, bool fullscreen);

signals:
    void telemetryChanged();
    void audioStateChanged();
    void audioDevicesChanged();
    void clockChanged();
    void monitorsChanged();

private slots:
    void pollTelemetry();
    void updateClock();

private:
    // Platform-specific hooks implemented in SystemIntegration_win.cpp / SystemIntegration_mac.cpp
    void platformInit();
    void platformShutdown();
    SystemTelemetry platformReadTelemetry();
    int platformGetVolumePercent() const;
    bool platformSetVolumePercent(int percent);
    bool platformGetMuted() const;
    bool platformSetMuted(bool muted);
    QList<AudioOutputDevice> platformListAudioOutputs() const;
    bool platformSelectAudioOutput(const QString &deviceId);
    bool platformOpenBluetoothSettings();
    bool platformOpenWifiSettings();
    bool platformOpenSoundSettings();
    bool platformSetStartupWithWindows(bool enabled);
    void platformApplyWindowStyle(QWindow *window);

    struct PlatformPrivate;
    PlatformPrivate *m_priv = nullptr;

    QTimer m_telemetryTimer;
    QTimer m_clockTimer;
    SystemTelemetry m_telemetry;
    int m_volumePercent = 50;
    bool m_muted = false;
    QList<AudioOutputDevice> m_audioDevices;
    QString m_currentTime;
    QString m_currentDate;
};

} // namespace Aether
