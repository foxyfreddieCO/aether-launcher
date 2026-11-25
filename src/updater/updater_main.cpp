#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <signal.h>
#endif

namespace {

void waitForPidExit(qint64 pid, int timeoutMs = 15000) {
    if (pid <= 0) return;
#ifdef Q_OS_WIN
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (h) {
        WaitForSingleObject(h, static_cast<DWORD>(timeoutMs));
        CloseHandle(h);
    }
#else
    int waited = 0;
    while (waited < timeoutMs) {
        if (kill(static_cast<pid_t>(pid), 0) != 0) break;
        QThread::msleep(150);
        waited += 150;
    }
#endif
    QThread::msleep(350);
}

bool copyDirectoryRecursive(const QString &srcDirPath, const QString &dstDirPath) {
    QDir srcDir(srcDirPath);
    if (!srcDir.exists()) return false;

    QDir().mkpath(dstDirPath);
    const QFileInfoList entries = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden);
    for (const QFileInfo &fi : entries) {
        const QString srcPath = fi.absoluteFilePath();
        const QString dstPath = QDir(dstDirPath).filePath(fi.fileName());
        if (fi.isDir()) {
            if (!copyDirectoryRecursive(srcPath, dstPath)) return false;
        } else {
            QFile::remove(dstPath);
            if (!QFile::copy(srcPath, dstPath)) return false;
        }
    }
    return true;
}

bool extractZipPackage(const QString &zipPath, const QString &extractDir) {
    QDir().mkpath(extractDir);
    QProcess proc;
#ifdef Q_OS_WIN
    // Windows 11 includes bsdtar (tar.exe) natively in System32
    proc.start(QStringLiteral("tar.exe"), {QStringLiteral("-xf"), zipPath, QStringLiteral("-C"), extractDir});
    if (!proc.waitForFinished(60000) || proc.exitCode() != 0) {
        // Fallback to PowerShell Expand-Archive
        proc.start(QStringLiteral("powershell.exe"), {
            QStringLiteral("-NoProfile"),
            QStringLiteral("-Command"),
            QStringLiteral("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force").arg(zipPath, extractDir)
        });
        if (!proc.waitForFinished(60000) || proc.exitCode() != 0) {
            return false;
        }
    }
#else
    proc.start(QStringLiteral("unzip"), {QStringLiteral("-o"), zipPath, QStringLiteral("-d"), extractDir});
    if (!proc.waitForFinished(60000) || proc.exitCode() != 0) {
        return false;
    }
#endif
    return true;
}

bool validateExtractedTree(const QString &extractRoot) {
    const QString canonicalRoot = QDir(extractRoot).canonicalPath();
    QDirIterator it(extractRoot, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (fi.isSymLink()) return false;
        const QString resolved = fi.canonicalFilePath();
        if (!resolved.startsWith(canonicalRoot)) return false;
    }
    return true;
}

bool launchAndVerifyHealth(const QString &targetExe, int timeoutMs = 15000) {
    const QString marker = QDir::temp().filePath(QStringLiteral("aether-health-") + QUuid::createUuid().toString(QUuid::Id128) + QStringLiteral(".ok"));
    QFile::remove(marker);

    if (!QProcess::startDetached(targetExe, {QStringLiteral("--update-healthcheck"), marker}, QFileInfo(targetExe).absolutePath())) {
        return false;
    }

    int waited = 0;
    while (waited < timeoutMs) {
        if (QFileInfo::exists(marker)) {
            QFile::remove(marker);
            return true;
        }
        QThread::msleep(250);
        waited += 250;
    }
    QFile::remove(marker);
    return false;
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("AetherUpdater"));

    QCommandLineParser parser;
    parser.addOption({QStringLiteral("pid"), QStringLiteral("Launcher PID to wait for"), QStringLiteral("pid")});
    parser.addOption({QStringLiteral("install-dir"), QStringLiteral("Application installation directory"), QStringLiteral("dir")});
    parser.addOption({QStringLiteral("backup-root"), QStringLiteral("Backup root directory"), QStringLiteral("dir")});
    parser.addOption({QStringLiteral("target-exe"), QStringLiteral("Launcher executable to restart"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("current-version"), QStringLiteral("Current version"), QStringLiteral("ver")});
    parser.addOption({QStringLiteral("new-version"), QStringLiteral("New version"), QStringLiteral("ver")});
    parser.addOption({QStringLiteral("package"), QStringLiteral("Verified zip package path"), QStringLiteral("zip")});
    parser.addOption({QStringLiteral("rollback-dir"), QStringLiteral("Explicit backup directory to restore"), QStringLiteral("dir")});
    parser.process(app);

    const qint64 pid = parser.value(QStringLiteral("pid")).toLongLong();
    const QString installDir = parser.value(QStringLiteral("install-dir"));
    const QString backupRoot = parser.value(QStringLiteral("backup-root"));
    const QString targetExe = parser.value(QStringLiteral("target-exe"));
    const QString currentVer = parser.value(QStringLiteral("current-version"));
    const QString packagePath = parser.value(QStringLiteral("package"));
    const QString rollbackDir = parser.value(QStringLiteral("rollback-dir"));

    if (installDir.isEmpty() || targetExe.isEmpty()) {
        return 2;
    }

    waitForPidExit(pid);

    // Manual rollback mode
    if (!rollbackDir.isEmpty()) {
        if (!copyDirectoryRecursive(rollbackDir, installDir)) {
            return 3;
        }
        QProcess::startDetached(targetExe, {}, installDir);
        return 0;
    }

    // Standard safe update mode:
    // 1. Backup existing installation binaries (never touches userDataDir!)
    const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString backupDir = QDir(backupRoot).filePath(QStringLiteral("v%1-%2").arg(currentVer.isEmpty() ? QStringLiteral("prev") : currentVer, stamp));
    if (!copyDirectoryRecursive(installDir, backupDir)) {
        QProcess::startDetached(targetExe, {}, installDir);
        return 4;
    }

    // 2. Extract and validate package in temp staging folder
    const QString extractDir = QDir::temp().filePath(QStringLiteral("aether-extract-") + QUuid::createUuid().toString(QUuid::Id128));
    bool ok = extractZipPackage(packagePath, extractDir) && validateExtractedTree(extractDir);

    // 3. Apply extracted files over installDir
    if (ok) {
        ok = copyDirectoryRecursive(extractDir, installDir);
    }
    QDir(extractDir).removeRecursively();

    // 4. Launch updated app with healthcheck handshake; rollback automatically if it fails!
    if (ok && launchAndVerifyHealth(targetExe, 15000)) {
        return 0;
    }

    // Automatic Rollback!
    copyDirectoryRecursive(backupDir, installDir);
    QProcess::startDetached(targetExe, {}, installDir);
    return 1;
}
