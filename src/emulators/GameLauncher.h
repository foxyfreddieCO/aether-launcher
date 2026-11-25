#pragma once

#include "EmulatorManager.h"
#include "core/Database.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QTimer>

namespace Aether {

struct ResolvedLaunchCommand {
    bool valid = false;
    QString errorReason;
    QString program;
    QStringList arguments;
    QString workingDirectory;
};

class GameLauncher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY stateChanged)
    Q_PROPERTY(QString activeGameId READ activeGameId NOTIFY stateChanged)
    Q_PROPERTY(QString activeGameTitle READ activeGameTitle NOTIFY stateChanged)
    Q_PROPERTY(qint64 activeSessionSeconds READ activeSessionSeconds NOTIFY sessionTick)
    Q_PROPERTY(QString lastError READ lastError NOTIFY launchFailed)

public:
    explicit GameLauncher(Database &database, EmulatorManager &emulatorManager, QObject *parent = nullptr);
    ~GameLauncher() override;

    bool isRunning() const { return m_process != nullptr && m_process->state() != QProcess::NotRunning; }
    QString activeGameId() const { return m_activeGameId; }
    QString activeGameTitle() const { return m_activeGameTitle; }
    qint64 activeSessionSeconds() const;
    QString lastError() const { return m_lastError; }

    ResolvedLaunchCommand resolveCommand(const GameEntry &game) const;
    Q_INVOKABLE QString buildLaunchPreview(const QString &gameId) const;
    Q_INVOKABLE bool launchGame(const QString &gameId);
    Q_INVOKABLE void forceStopActiveGame();

signals:
    void stateChanged();
    void sessionTick();
    void gameLaunched(const QString &gameId, const QString &gameTitle);
    void gameExited(const QString &gameId, const QString &gameTitle, qint64 sessionSeconds, int exitCode);
    void launchFailed(const QString &gameTitle, const QString &errorReason);
    void sessionRecorded(const QString &gameId);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void cleanupProcess();

    Database &m_db;
    EmulatorManager &m_emulators;
    QProcess *m_process = nullptr;
    QTimer m_sessionTimer;
    QElapsedTimer m_elapsed;
    QDateTime m_startedAt;
    QString m_activeGameId;
    QString m_activeGameTitle;
    QString m_lastError;
    bool m_errorAlreadyReported = false;
};

} // namespace Aether
