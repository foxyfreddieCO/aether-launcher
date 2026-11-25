#include "SystemIntegration.h"

#include <QDateTime>
#include <QGuiApplication>
#include <QScreen>

namespace Aether {

SystemIntegration::SystemIntegration(QObject *parent)
    : QObject(parent) {
    platformInit();

    m_telemetryTimer.setInterval(2000);
    connect(&m_telemetryTimer, &QTimer::timeout, this, &SystemIntegration::pollTelemetry);

    m_clockTimer.setInterval(1000);
    connect(&m_clockTimer, &QTimer::timeout, this, &SystemIntegration::updateClock);

    if (qGuiApp) {
        connect(qGuiApp, &QGuiApplication::screenAdded, this, &SystemIntegration::monitorsChanged);
        connect(qGuiApp, &QGuiApplication::screenRemoved, this, &SystemIntegration::monitorsChanged);
    }

    updateClock();
}

SystemIntegration::~SystemIntegration() {
    stopMonitoring();
    platformShutdown();
}

void SystemIntegration::startMonitoring() {
    updateClock();
    pollTelemetry();
    refreshAudioDevices();
    m_telemetryTimer.start();
    m_clockTimer.start();
}

void SystemIntegration::stopMonitoring() {
    m_telemetryTimer.stop();
    m_clockTimer.stop();
}

void SystemIntegration::pollTelemetry() {
    m_telemetry = platformReadTelemetry();
    emit telemetryChanged();

    const int vol = platformGetVolumePercent();
    const bool muted = platformGetMuted();
    if (vol >= 0 && (vol != m_volumePercent || muted != m_muted)) {
        m_volumePercent = vol;
        m_muted = muted;
        emit audioStateChanged();
    }
}

void SystemIntegration::updateClock() {
    const QDateTime now = QDateTime::currentDateTime();
    const QString t = now.toString(QStringLiteral("h:mm AP"));
    const QString d = now.toString(QStringLiteral("ddd, MMM d"));
    if (t != m_currentTime || d != m_currentDate) {
        m_currentTime = t;
        m_currentDate = d;
        emit clockChanged();
    }
}

void SystemIntegration::setSystemVolume(int percent) {
    percent = qBound(0, percent, 100);
    if (platformSetVolumePercent(percent)) {
        m_volumePercent = percent;
        emit audioStateChanged();
    }
}

void SystemIntegration::setSystemMuted(bool muted) {
    if (platformSetMuted(muted)) {
        m_muted = muted;
        emit audioStateChanged();
    }
}

QVariantList SystemIntegration::audioOutputDevicesVariant() const {
    QVariantList list;
    for (const auto &d : m_audioDevices) {
        list.append(d.toVariantMap());
    }
    return list;
}

QString SystemIntegration::currentAudioDeviceName() const {
    for (const auto &d : m_audioDevices) {
        if (d.isDefault) return d.name;
    }
    return m_audioDevices.isEmpty() ? QStringLiteral("System Default") : m_audioDevices.first().name;
}

void SystemIntegration::refreshAudioDevices() {
    m_audioDevices = platformListAudioOutputs();
    emit audioDevicesChanged();
}

bool SystemIntegration::selectAudioOutputDevice(const QString &deviceId) {
    const bool ok = platformSelectAudioOutput(deviceId);
    refreshAudioDevices();
    return ok;
}

bool SystemIntegration::openBluetoothSettings() {
    return platformOpenBluetoothSettings();
}

bool SystemIntegration::openWifiSettings() {
    return platformOpenWifiSettings();
}

bool SystemIntegration::openSoundSettings() {
    return platformOpenSoundSettings();
}

bool SystemIntegration::setStartupWithWindows(bool enabled) {
    return platformSetStartupWithWindows(enabled);
}

void SystemIntegration::applyWindowStyle(QWindow *window) {
    if (window) {
        platformApplyWindowStyle(window);
    }
}

QVariantList SystemIntegration::monitorsVariant() const {
    QVariantList out;
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *s = screens.at(i);
        const QRect g = s->geometry();
        out.append(QVariantMap{
            {QStringLiteral("index"), i},
            {QStringLiteral("name"), s->name()},
            {QStringLiteral("resolution"), QStringLiteral("%1x%2").arg(g.width()).arg(g.height())},
            {QStringLiteral("refreshRate"), qRound(s->refreshRate())},
            {QStringLiteral("isPrimary"), s == QGuiApplication::primaryScreen()}
        });
    }
    return out;
}

void SystemIntegration::moveWindowToMonitor(QWindow *window, int monitorIndex, bool fullscreen) {
    if (!window) return;
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) return;

    const int safeIdx = qBound(0, monitorIndex, static_cast<int>(screens.size()) - 1);
    QScreen *target = screens.at(safeIdx);
    window->setScreen(target);
    const QRect geom = target->availableGeometry();

    if (fullscreen) {
        window->setGeometry(target->geometry());
        window->showFullScreen();
    } else {
        window->showNormal();
        const int w = qMin(1440, geom.width() - 80);
        const int h = qMin(860, geom.height() - 80);
        window->setGeometry(
            geom.x() + (geom.width() - w) / 2,
            geom.y() + (geom.height() - h) / 2,
            w,
            h);
    }
}

QString SystemIntegration::platformName() const {
#ifdef Q_OS_WIN
    return QStringLiteral("Windows 11");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macOS (Dev Mode)");
#else
    return QStringLiteral("Linux");
#endif
}

} // namespace Aether
