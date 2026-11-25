#include "UpdateService.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QUrl>

namespace Aether {

UpdateService::UpdateService(const AppPaths &paths, SettingsManager &settings, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_settings(settings) {}

int UpdateService::compareVersions(const QString &v1, const QString &v2) {
    auto parse = [](QString s) -> QList<int> {
        s = s.trimmed();
        if (s.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) s = s.mid(1);
        const int dash = s.indexOf(QLatin1Char('-'));
        if (dash >= 0) s = s.left(dash);
        const QStringList parts = s.split(QLatin1Char('.'));
        QList<int> nums;
        for (const QString &p : parts) {
            bool ok = false;
            nums.append(p.toInt(&ok));
        }
        while (nums.size() < 3) nums.append(0);
        return nums;
    };

    const QList<int> a = parse(v1);
    const QList<int> b = parse(v2);
    const int n = qMax(a.size(), b.size());
    for (int i = 0; i < n; ++i) {
        const int va = (i < a.size()) ? a.at(i) : 0;
        const int vb = (i < b.size()) ? b.at(i) : 0;
        if (va < vb) return -1;
        if (va > vb) return 1;
    }
    return 0;
}

QString UpdateService::computeFileSha256(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f)) return {};
    return QString::fromLatin1(hash.result().toHex());
}

bool UpdateService::verifySha256(const QString &filePath, const QString &expectedHex) {
    const QString cleaned = expectedHex.trimmed().toLower();
    if (cleaned.isEmpty()) return false;
    return computeFileSha256(filePath).toLower() == cleaned;
}

bool UpdateService::writeHealthcheckMarker(const QString &markerPath) {
    if (markerPath.isEmpty()) return false;
    QFile f(markerPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write("OK\n");
    f.flush();
    return true;
}

QVariantList UpdateService::installBackupsVariant() const {
    const QDir dir(m_paths.updatesStagingDir() + QStringLiteral("/install_backups"));
    QVariantList out;
    if (!dir.exists()) return out;

    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (const auto &fi : entries) {
        out.append(QVariantMap{
            {QStringLiteral("name"), fi.fileName()},
            {QStringLiteral("path"), fi.absoluteFilePath()},
            {QStringLiteral("date"), fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"))}
        });
    }
    return out;
}

void UpdateService::refreshInstallBackups() {
    emit backupsChanged();
}

void UpdateService::checkForUpdates() {
    if (m_checking || m_downloading) return;

    const QString urlStr = m_settings.updateManifestUrl().trimmed();
    if (urlStr.isEmpty()) {
        m_statusMessage = QStringLiteral("No update feed URL configured. Set your GitHub Releases manifest or Local Dev Companion URL in Settings -> Updates.");
        m_updateAvailable = false;
        emit stateChanged();
        return;
    }

    const QUrl url(urlStr);
    if (!url.isValid() || (url.scheme() != QLatin1String("https") && url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("file"))) {
        m_statusMessage = QStringLiteral("Invalid update manifest URL.");
        emit stateChanged();
        return;
    }

    m_checking = true;
    m_statusMessage = QStringLiteral("Checking for updates...");
    emit stateChanged();

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AetherLauncher/%1").arg(currentVersion()));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_checking = false;

        if (reply->error() != QNetworkReply::NoError) {
            m_statusMessage = QStringLiteral("Update check failed: %1").arg(reply->errorString());
            emit stateChanged();
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            m_statusMessage = QStringLiteral("Update manifest format is invalid JSON.");
            emit stateChanged();
            return;
        }

        const UpdateManifest manifest = UpdateManifest::fromJson(doc.object());
        if (!manifest.valid) {
            m_statusMessage = QStringLiteral("Update manifest is missing required fields (version, downloadUrl).");
            emit stateChanged();
            return;
        }

        m_manifest = manifest;
        if (compareVersions(manifest.version, currentVersion()) > 0) {
            m_updateAvailable = true;
            m_statusMessage = QStringLiteral("Update v%1 is available!").arg(manifest.version);
            emit stateChanged();
            emit updateAvailableNotified(manifest.version, manifest.releaseNotes);
        } else {
            m_updateAvailable = false;
            m_statusMessage = QStringLiteral("You are running the latest version (v%1).").arg(currentVersion());
            emit stateChanged();
        }
    });
}

void UpdateService::downloadAndInstallUpdate() {
    if (!m_updateAvailable || !m_manifest.valid || m_downloading) return;

    const QUrl url(m_manifest.downloadUrl);
    if (!url.isValid()) {
        m_statusMessage = QStringLiteral("Invalid package download URL in manifest.");
        emit stateChanged();
        return;
    }

    m_paths.ensureDirectoriesExist();
    m_downloading = true;
    m_downloadProgress = 0.0;
    m_statusMessage = QStringLiteral("Downloading v%1...").arg(m_manifest.version);
    emit stateChanged();
    emit downloadProgressChanged();

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AetherLauncher/%1").arg(currentVersion()));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_downloadProgress = static_cast<double>(received) / static_cast<double>(total);
            emit downloadProgressChanged();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_downloading = false;

        if (reply->error() != QNetworkReply::NoError) {
            m_statusMessage = QStringLiteral("Download failed: %1").arg(reply->errorString());
            emit stateChanged();
            return;
        }

        const QString pkgPath = QDir(m_paths.updatesStagingDir())
            .filePath(QStringLiteral("update-%1.zip").arg(AppPaths::sanitizeFilename(m_manifest.version)));

        QFile outFile(pkgPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            m_statusMessage = QStringLiteral("Could not write update package to staging folder.");
            emit stateChanged();
            return;
        }
        outFile.write(reply->readAll());
        outFile.flush();
        outFile.close();

        // Mandatory SHA-256 verification when hash is provided
        if (!m_manifest.sha256.trimmed().isEmpty()) {
            m_statusMessage = QStringLiteral("Verifying SHA-256 package signature...");
            emit stateChanged();
            if (!verifySha256(pkgPath, m_manifest.sha256)) {
                QFile::remove(pkgPath);
                m_statusMessage = QStringLiteral("Security check failed: SHA-256 checksum mismatch. Update aborted.");
                emit stateChanged();
                return;
            }
        }

        m_statusMessage = QStringLiteral("Launching updater with automatic backup & rollback protection...");
        emit stateChanged();

        if (launchUpdaterProcess(pkgPath)) {
            emit readyToRestartForUpdate();
            QCoreApplication::quit();
        } else {
            m_statusMessage = QStringLiteral("Could not launch AetherUpdater executable alongside AetherLauncher.");
            emit stateChanged();
        }
    });
}

bool UpdateService::rollbackToInstallBackup(const QString &backupDir) {
    if (backupDir.isEmpty() || !QDir(backupDir).exists()) {
        m_statusMessage = QStringLiteral("Selected backup directory does not exist.");
        emit stateChanged();
        return false;
    }

    if (launchUpdaterProcess(QString(), backupDir)) {
        emit readyToRestartForUpdate();
        QCoreApplication::quit();
        return true;
    }
    m_statusMessage = QStringLiteral("Could not launch AetherUpdater for rollback.");
    emit stateChanged();
    return false;
}

bool UpdateService::launchUpdaterProcess(const QString &packagePath, const QString &rollbackDir) {
    const QString installDir = m_paths.installDir();
#ifdef Q_OS_WIN
    const QString updaterName = QStringLiteral("AetherUpdater.exe");
#else
    const QString updaterName = QStringLiteral("AetherUpdater");
#endif
    const QString updaterSrc = QDir(installDir).filePath(updaterName);
    if (!QFileInfo::exists(updaterSrc)) {
        return false;
    }

    // Copy updater to staging directory before running so Windows never locks the executable inside installDir
    m_paths.ensureDirectoriesExist();
    const QString updaterCopy = QDir(m_paths.updatesStagingDir()).filePath(updaterName);
    QFile::remove(updaterCopy);
    if (!QFile::copy(updaterSrc, updaterCopy)) {
        return false;
    }
#ifndef Q_OS_WIN
    QFile::setPermissions(updaterCopy, QFile::permissions(updaterSrc));
#endif

    const QString backupRoot = m_paths.updatesStagingDir() + QStringLiteral("/install_backups");
    QStringList args = {
        QStringLiteral("--pid"), QString::number(QCoreApplication::applicationPid()),
        QStringLiteral("--install-dir"), installDir,
        QStringLiteral("--backup-root"), backupRoot,
        QStringLiteral("--target-exe"), QCoreApplication::applicationFilePath(),
        QStringLiteral("--current-version"), currentVersion()
    };

    if (!rollbackDir.isEmpty()) {
        args << QStringLiteral("--rollback-dir") << rollbackDir;
    } else {
        args << QStringLiteral("--package") << packagePath
             << QStringLiteral("--new-version") << m_manifest.version;
    }

    return QProcess::startDetached(updaterCopy, args, m_paths.updatesStagingDir());
}

} // namespace Aether
