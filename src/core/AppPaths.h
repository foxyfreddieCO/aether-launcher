#pragma once

#include <QString>

namespace Aether {

class AppPaths {
public:
    explicit AppPaths(const QString &overrideDataDir = QString());

    QString installDir() const;
    QString userDataDir() const;
    QString databaseFile() const;
    QString settingsFile() const;
    QString artworkDir() const;
    QString coversDir() const;
    QString backgroundsDir() const;
    QString backupsDir() const;
    QString themesDir() const;
    QString soundPacksDir() const;
    QString audioCacheDir() const;
    QString updatesStagingDir() const;
    QString logsDir() const;

    bool ensureDirectoriesExist() const;

    // Security helper: validates that resolvedPath stays strictly within baseDir
    static bool isPathInsideDirectory(const QString &baseDir, const QString &candidatePath);
    static QString sanitizeFilename(const QString &input);

private:
    QString m_dataDir;
};

} // namespace Aether
