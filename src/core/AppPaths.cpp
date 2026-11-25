#include "AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

namespace Aether {

AppPaths::AppPaths(const QString &overrideDataDir) {
    if (!overrideDataDir.isEmpty()) {
        m_dataDir = QDir::cleanPath(overrideDataDir);
    } else {
        const QString envOverride = qEnvironmentVariable("AETHER_DATA_DIR");
        if (!envOverride.isEmpty()) {
            m_dataDir = QDir::cleanPath(envOverride);
        } else {
            // Strictly outside the installation directory:
            // Windows 11: %LOCALAPPDATA%/AetherLauncher
            // macOS:      ~/Library/Application Support/AetherLauncher
            QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
            if (base.isEmpty()) {
                base = QDir::homePath() + QStringLiteral("/.aetherlauncher");
            }
            m_dataDir = QDir::cleanPath(base);
        }
    }
}

QString AppPaths::installDir() const {
    return QCoreApplication::applicationDirPath();
}

QString AppPaths::userDataDir() const {
    return m_dataDir;
}

QString AppPaths::databaseFile() const {
    return m_dataDir + QStringLiteral("/database/library.sqlite");
}

QString AppPaths::settingsFile() const {
    return m_dataDir + QStringLiteral("/settings.json");
}

QString AppPaths::artworkDir() const {
    return m_dataDir + QStringLiteral("/artwork");
}

QString AppPaths::coversDir() const {
    return artworkDir() + QStringLiteral("/covers");
}

QString AppPaths::backgroundsDir() const {
    return artworkDir() + QStringLiteral("/backgrounds");
}

QString AppPaths::backupsDir() const {
    return m_dataDir + QStringLiteral("/backups");
}

QString AppPaths::themesDir() const {
    return m_dataDir + QStringLiteral("/themes");
}

QString AppPaths::soundPacksDir() const {
    return m_dataDir + QStringLiteral("/soundpacks");
}

QString AppPaths::audioCacheDir() const {
    return m_dataDir + QStringLiteral("/cache/audio");
}

QString AppPaths::updatesStagingDir() const {
    return m_dataDir + QStringLiteral("/updates");
}

QString AppPaths::logsDir() const {
    return m_dataDir + QStringLiteral("/logs");
}

bool AppPaths::ensureDirectoriesExist() const {
    const QStringList dirs = {
        userDataDir(),
        m_dataDir + QStringLiteral("/database"),
        artworkDir(),
        coversDir(),
        backgroundsDir(),
        backupsDir(),
        themesDir(),
        soundPacksDir(),
        audioCacheDir(),
        updatesStagingDir(),
        logsDir()
    };

    QDir dir;
    for (const QString &d : dirs) {
        if (!dir.mkpath(d)) {
            return false;
        }
    }
    return true;
}

bool AppPaths::isPathInsideDirectory(const QString &baseDir, const QString &candidatePath) {
    QDir base(QFileInfo(baseDir).absoluteFilePath());
    const QString canonicalBase = base.canonicalPath().isEmpty()
        ? QDir::cleanPath(base.absolutePath())
        : base.canonicalPath();

    // Canonicalize candidatePath (or its nearest existing ancestor if the file does not exist yet)
    QFileInfo candInfo(QDir::cleanPath(QFileInfo(candidatePath).absoluteFilePath()));
    QString absCandidate = candInfo.canonicalFilePath();
    if (absCandidate.isEmpty()) {
        const QString parentCanon = QDir(candInfo.absolutePath()).canonicalPath();
        absCandidate = parentCanon.isEmpty()
            ? QDir::cleanPath(candInfo.absoluteFilePath())
            : QDir::cleanPath(parentCanon + QLatin1Char('/') + candInfo.fileName());
    }

    if (absCandidate == canonicalBase) {
        return true;
    }

    QString prefix = canonicalBase;
    if (!prefix.endsWith(QLatin1Char('/'))) {
        prefix += QLatin1Char('/');
    }
#ifdef Q_OS_WIN
    return absCandidate.startsWith(prefix, Qt::CaseInsensitive);
#else
    return absCandidate.startsWith(prefix, Qt::CaseSensitive);
#endif
}

QString AppPaths::sanitizeFilename(const QString &input) {
    QString cleaned = input.trimmed();
    cleaned.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|\r\n\t]+)")), QStringLiteral("_"));
    if (cleaned.isEmpty()) {
        cleaned = QStringLiteral("item");
    }
    return cleaned.left(120);
}

} // namespace Aether
