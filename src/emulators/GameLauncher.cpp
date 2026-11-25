#include "GameLauncher.h"

#include <QDir>
#include <QFileInfo>

namespace Aether {

GameLauncher::GameLauncher(Database &database, EmulatorManager &emulatorManager, QObject *parent)
    : QObject(parent)
    , m_db(database)
    , m_emulators(emulatorManager) {
    m_sessionTimer.setInterval(1000);
    connect(&m_sessionTimer, &QTimer::timeout, this, &GameLauncher::sessionTick);
}

GameLauncher::~GameLauncher() {
    if (m_process) {
        m_process->disconnect(this);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            m_process->waitForFinished(1500);
        }
        delete m_process;
        m_process = nullptr;
    }
}

qint64 GameLauncher::activeSessionSeconds() const {
    if (!isRunning() || !m_elapsed.isValid()) return 0;
    return m_elapsed.elapsed() / 1000;
}

ResolvedLaunchCommand GameLauncher::resolveCommand(const GameEntry &game) const {
    ResolvedLaunchCommand cmd;

    if (game.executablePath.trimmed().isEmpty()) {
        cmd.errorReason = QStringLiteral("No game executable or ROM file path is configured for \"%1\".").arg(game.title);
        return cmd;
    }

    if (!QFileInfo::exists(game.executablePath)) {
        cmd.errorReason = QStringLiteral("Game file not found at:\n%1").arg(QDir::toNativeSeparators(game.executablePath));
        return cmd;
    }

    if (game.emulatorId.isEmpty()) {
        // Direct native Windows executable (.exe) or platform executable
        cmd.program = game.executablePath;
        if (!game.customArguments.trimmed().isEmpty()) {
            cmd.arguments = EmulatorManager::parseArgumentsTemplate(game.customArguments, game.executablePath, game.title);
        }
        cmd.workingDirectory = !game.workingDirectory.isEmpty()
            ? game.workingDirectory
            : QFileInfo(game.executablePath).absolutePath();
        cmd.valid = true;
        return cmd;
    }

    // Emulated game launch
    const auto emuOpt = m_emulators.findById(game.emulatorId);
    if (!emuOpt.has_value()) {
        cmd.errorReason = QStringLiteral("Assigned emulator profile (%1) no longer exists. Reassign an emulator in Game Settings.").arg(game.emulatorId);
        return cmd;
    }

    const EmulatorProfile &emu = *emuOpt;
    if (emu.executablePath.trimmed().isEmpty()) {
        cmd.errorReason = QStringLiteral("Emulator \"%1\" does not have an executable path configured yet. Open Emulators settings and browse to its executable.").arg(emu.name);
        return cmd;
    }

    if (!QFileInfo::exists(emu.executablePath)) {
        cmd.errorReason = QStringLiteral("Emulator \"%1\" executable not found at:\n%2").arg(emu.name, QDir::toNativeSeparators(emu.executablePath));
        return cmd;
    }

    const QString argTemplate = !game.customArguments.trimmed().isEmpty()
        ? game.customArguments
        : (emu.launchArguments.trimmed().isEmpty() ? QStringLiteral("\"{rom}\"") : emu.launchArguments);

    cmd.program = emu.executablePath;
    cmd.arguments = EmulatorManager::parseArgumentsTemplate(argTemplate, game.executablePath, game.title);

    if (!game.workingDirectory.isEmpty()) {
        cmd.workingDirectory = game.workingDirectory;
    } else if (!emu.workingDirectory.isEmpty()) {
        cmd.workingDirectory = emu.workingDirectory;
    } else {
        cmd.workingDirectory = QFileInfo(emu.executablePath).absolutePath();
    }

    cmd.valid = true;
    return cmd;
}

QString GameLauncher::buildLaunchPreview(const QString &gameId) const {
    const auto gameOpt = m_db.getGame(gameId);
    if (!gameOpt.has_value()) return {};

    const ResolvedLaunchCommand cmd = resolveCommand(*gameOpt);
    if (!cmd.valid) {
        return QStringLiteral("[Configuration Required] ") + cmd.errorReason;
    }

    QStringList parts;
    parts.append(QStringLiteral("\"%1\"").arg(QDir::toNativeSeparators(cmd.program)));
    for (const QString &arg : cmd.arguments) {
        if (arg.contains(QLatin1Char(' '))) {
            parts.append(QStringLiteral("\"%1\"").arg(arg));
        } else {
            parts.append(arg);
        }
    }
    return parts.join(QLatin1Char(' '));
}

bool GameLauncher::launchGame(const QString &gameId) {
    if (isRunning()) {
        m_lastError = QStringLiteral("\"%1\" is already running. Close it before launching another game.").arg(m_activeGameTitle);
        emit launchFailed(m_activeGameTitle, m_lastError);
        return false;
    }

    const auto gameOpt = m_db.getGame(gameId);
    if (!gameOpt.has_value()) {
        m_lastError = QStringLiteral("Selected game was not found in the library.");
        emit launchFailed(QStringLiteral("Unknown Game"), m_lastError);
        return false;
    }

    const GameEntry &game = *gameOpt;
    const ResolvedLaunchCommand cmd = resolveCommand(game);
    if (!cmd.valid) {
        m_lastError = cmd.errorReason;
        emit launchFailed(game.title, m_lastError);
        return false;
    }

    cleanupProcess();

    m_process = new QProcess(this);
    m_process->setProgram(cmd.program);
    m_process->setArguments(cmd.arguments);
    if (!cmd.workingDirectory.isEmpty() && QDir(cmd.workingDirectory).exists()) {
        m_process->setWorkingDirectory(cmd.workingDirectory);
    }

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GameLauncher::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &GameLauncher::onProcessError);

    m_activeGameId = game.id;
    m_activeGameTitle = game.title;
    m_lastError.clear();
    m_errorAlreadyReported = false;
    m_startedAt = QDateTime::currentDateTimeUtc();
    m_elapsed.start();

    m_process->start();
    if (!m_process->waitForStarted(4000)) {
        const QString err = m_process->errorString();
        m_lastError = QStringLiteral("Failed to launch \"%1\": %2").arg(game.title, err);
        m_errorAlreadyReported = true;
        cleanupProcess();
        m_activeGameId.clear();
        m_activeGameTitle.clear();
        emit stateChanged();
        emit launchFailed(game.title, m_lastError);
        return false;
    }

    m_sessionTimer.start();
    emit stateChanged();
    emit gameLaunched(game.id, game.title);
    return true;
}

void GameLauncher::forceStopActiveGame() {
    if (!m_process || m_process->state() == QProcess::NotRunning) return;
    m_process->terminate();
    if (!m_process->waitForFinished(2500)) {
        m_process->kill();
    }
}

void GameLauncher::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus)
    m_sessionTimer.stop();

    const qint64 durationSeconds = m_elapsed.isValid() ? qMax<qint64>(1, m_elapsed.elapsed() / 1000) : 0;
    const QDateTime endedAt = QDateTime::currentDateTimeUtc();
    const QString finishedId = m_activeGameId;
    const QString finishedTitle = m_activeGameTitle;

    if (!finishedId.isEmpty()) {
        m_db.recordPlaySession(finishedId, durationSeconds, m_startedAt, endedAt);
        emit sessionRecorded(finishedId);
    }

    cleanupProcess();
    m_activeGameId.clear();
    m_activeGameTitle.clear();

    emit stateChanged();
    emit gameExited(finishedId, finishedTitle, durationSeconds, exitCode);
}

void GameLauncher::onProcessError(QProcess::ProcessError error) {
    if (m_errorAlreadyReported) return;
    if (error == QProcess::FailedToStart || error == QProcess::Crashed) {
        const QString title = m_activeGameTitle.isEmpty() ? QStringLiteral("Game") : m_activeGameTitle;
        m_lastError = (error == QProcess::FailedToStart)
            ? QStringLiteral("Could not start process for \"%1\". Verify executable permissions and path.").arg(title)
            : QStringLiteral("\"%1\" exited unexpectedly.").arg(title);
        m_errorAlreadyReported = true;
        emit launchFailed(title, m_lastError);
    }
}

void GameLauncher::cleanupProcess() {
    if (m_process) {
        m_process->disconnect(this);
        m_process->deleteLater();
        m_process = nullptr;
    }
}

} // namespace Aether
