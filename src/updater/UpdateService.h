#pragma once

#include "core/AppPaths.h"
#include "core/Models.h"
#include "core/SettingsManager.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace Aether {

class UpdateService : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(bool checking READ isChecking NOTIFY stateChanged)
    Q_PROPERTY(bool updateAvailable READ isUpdateAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool downloading READ isDownloading NOTIFY stateChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap availableManifest READ availableManifestVariant NOTIFY stateChanged)
    Q_PROPERTY(QVariantList installBackups READ installBackupsVariant NOTIFY backupsChanged)

public:
#ifndef AETHER_APP_VERSION
#define AETHER_APP_VERSION "1.0.0"
#endif
    static constexpr const char *kAppVersion = AETHER_APP_VERSION;

    explicit UpdateService(const AppPaths &paths, SettingsManager &settings, QObject *parent = nullptr);

    QString currentVersion() const { return QString::fromLatin1(kAppVersion); }
    bool isChecking() const { return m_checking; }
    bool isUpdateAvailable() const { return m_updateAvailable; }
    bool isDownloading() const { return m_downloading; }
    double downloadProgress() const { return m_downloadProgress; }
    QString statusMessage() const { return m_statusMessage; }
    QVariantMap availableManifestVariant() const { return m_manifest.toVariantMap(); }
    QVariantList installBackupsVariant() const;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadAndInstallUpdate();
    Q_INVOKABLE bool rollbackToInstallBackup(const QString &backupDir);
    Q_INVOKABLE void refreshInstallBackups();

    // Pure helpers (unit-tested!)
    static int compareVersions(const QString &v1, const QString &v2);
    static QString computeFileSha256(const QString &filePath);
    static bool verifySha256(const QString &filePath, const QString &expectedHex);
    static bool writeHealthcheckMarker(const QString &markerPath);

signals:
    void stateChanged();
    void downloadProgressChanged();
    void backupsChanged();
    void updateAvailableNotified(const QString &version, const QString &notes);
    void readyToRestartForUpdate();

private:
    bool launchUpdaterProcess(const QString &packagePath, const QString &rollbackDir = QString());

    AppPaths m_paths;
    SettingsManager &m_settings;
    QNetworkAccessManager m_net;

    bool m_checking = false;
    bool m_updateAvailable = false;
    bool m_downloading = false;
    double m_downloadProgress = 0.0;
    QString m_statusMessage = QStringLiteral("Up to date");
    UpdateManifest m_manifest;
};

} // namespace Aether
