#include "SystemIntegration.h"

#ifdef Q_OS_MACOS

#include <QAudioDevice>
#include <QDesktopServices>
#include <QMediaDevices>
#include <QProcess>
#include <QUrl>

#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <sys/sysctl.h>

namespace Aether {

struct SystemIntegration::PlatformPrivate {
    uint64_t prevTotalTicks = 0;
    uint64_t prevIdleTicks = 0;
    bool hasPrevCpu = false;
    double totalRamGb = 0.0;
};

void SystemIntegration::platformInit() {
    m_priv = new PlatformPrivate();
    uint64_t memSize = 0;
    size_t len = sizeof(memSize);
    if (sysctlbyname("hw.memsize", &memSize, &len, nullptr, 0) == 0) {
        m_priv->totalRamGb = static_cast<double>(memSize) / (1024.0 * 1024.0 * 1024.0);
    }
}

void SystemIntegration::platformShutdown() {
    delete m_priv;
    m_priv = nullptr;
}

SystemTelemetry SystemIntegration::platformReadTelemetry() {
    SystemTelemetry t;

    // CPU via Mach host_statistics
    host_cpu_load_info_data_t cpuInfo{};
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, reinterpret_cast<host_info_t>(&cpuInfo), &count) == KERN_SUCCESS) {
        const uint64_t totalTicks =
            cpuInfo.cpu_ticks[CPU_STATE_USER] +
            cpuInfo.cpu_ticks[CPU_STATE_SYSTEM] +
            cpuInfo.cpu_ticks[CPU_STATE_NICE] +
            cpuInfo.cpu_ticks[CPU_STATE_IDLE];
        const uint64_t idleTicks = cpuInfo.cpu_ticks[CPU_STATE_IDLE];

        if (m_priv && m_priv->hasPrevCpu && totalTicks > m_priv->prevTotalTicks) {
            const uint64_t dTotal = totalTicks - m_priv->prevTotalTicks;
            const uint64_t dIdle = idleTicks - m_priv->prevIdleTicks;
            t.cpuUsagePercent = qBound(0.0, (1.0 - static_cast<double>(dIdle) / static_cast<double>(dTotal)) * 100.0, 100.0);
        }
        if (m_priv) {
            m_priv->prevTotalTicks = totalTicks;
            m_priv->prevIdleTicks = idleTicks;
            m_priv->hasPrevCpu = true;
        }
    }

    // RAM via vm_statistics64
    vm_size_t pageSize = 4096;
    host_page_size(mach_host_self(), &pageSize);
    vm_statistics64_data_t vmStat{};
    mach_msg_type_number_t vmCount = HOST_VM_INFO64_COUNT;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vmStat), &vmCount) == KERN_SUCCESS) {
        const double usedBytes = static_cast<double>(vmStat.active_count + vmStat.wire_count + vmStat.compressor_page_count) * static_cast<double>(pageSize);
        t.ramTotalGb = m_priv ? m_priv->totalRamGb : 0.0;
        t.ramUsedGb = usedBytes / (1024.0 * 1024.0 * 1024.0);
        if (t.ramTotalGb > 0.0) {
            t.ramUsagePercent = qBound(0.0, (t.ramUsedGb / t.ramTotalGb) * 100.0, 100.0);
        }
    }

    t.gpuAvailable = false;
    t.temperatureAvailable = false;
    return t;
}

int SystemIntegration::platformGetVolumePercent() const {
    QProcess p;
    p.start(QStringLiteral("osascript"), {QStringLiteral("-e"), QStringLiteral("output volume of (get volume settings)")});
    if (p.waitForFinished(400)) {
        bool ok = false;
        const int val = QString::fromUtf8(p.readAllStandardOutput()).trimmed().toInt(&ok);
        if (ok) return qBound(0, val, 100);
    }
    return -1;
}

bool SystemIntegration::platformSetVolumePercent(int percent) {
    return QProcess::startDetached(
        QStringLiteral("osascript"),
        {QStringLiteral("-e"), QStringLiteral("set volume output volume %1").arg(qBound(0, percent, 100))});
}

bool SystemIntegration::platformGetMuted() const {
    QProcess p;
    p.start(QStringLiteral("osascript"), {QStringLiteral("-e"), QStringLiteral("output muted of (get volume settings)")});
    if (p.waitForFinished(400)) {
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed() == QLatin1String("true");
    }
    return false;
}

bool SystemIntegration::platformSetMuted(bool muted) {
    return QProcess::startDetached(
        QStringLiteral("osascript"),
        {QStringLiteral("-e"), QStringLiteral("set volume output muted %1").arg(muted ? QStringLiteral("true") : QStringLiteral("false"))});
}

QList<AudioOutputDevice> SystemIntegration::platformListAudioOutputs() const {
    QList<AudioOutputDevice> out;
    const QAudioDevice def = QMediaDevices::defaultAudioOutput();
    const auto list = QMediaDevices::audioOutputs();
    for (const auto &dev : list) {
        AudioOutputDevice item;
        item.id = QString::fromUtf8(dev.id());
        item.name = dev.description();
        item.isDefault = (dev.id() == def.id());
        out.append(item);
    }
    return out;
}

bool SystemIntegration::platformSelectAudioOutput(const QString &deviceId) {
    Q_UNUSED(deviceId)
    // macOS does not provide a public non-privileged API to change system default output without CoreAudio device matching;
    // open Sound Settings cleanly on macOS dev host. On Windows 11, WASAPI IPolicyConfig switches devices directly.
    return platformOpenSoundSettings();
}

bool SystemIntegration::platformOpenBluetoothSettings() {
    return QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.BluetoothSettings")));
}

bool SystemIntegration::platformOpenWifiSettings() {
    return QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.wifi-settings-extension")));
}

bool SystemIntegration::platformOpenSoundSettings() {
    return QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.Sound-Settings.extension")));
}

bool SystemIntegration::platformSetStartupWithWindows(bool enabled) {
    Q_UNUSED(enabled)
    return true; // Saved in settings; registry write is performed on Windows 11
}

void SystemIntegration::platformApplyWindowStyle(QWindow *window) {
    Q_UNUSED(window)
}

} // namespace Aether

#endif // Q_OS_MACOS
