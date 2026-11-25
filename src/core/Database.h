#pragma once

#include "Models.h"

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <optional>

namespace Aether {

class Database {
public:
    explicit Database(const QString &dbPath);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open();
    void close();
    bool isOpen() const;
    QString lastError() const { return m_lastError; }
    QString databasePath() const { return m_dbPath; }

    // Games
    bool upsertGame(const GameEntry &game);
    bool removeGame(const QString &gameId);
    std::optional<GameEntry> getGame(const QString &gameId) const;
    QList<GameEntry> getAllGames() const;
    bool recordPlaySession(const QString &gameId, qint64 durationSeconds, const QDateTime &startedAt, const QDateTime &endedAt);

    // Emulators
    bool upsertEmulator(const EmulatorProfile &emulator);
    bool removeEmulator(const QString &emulatorId);
    std::optional<EmulatorProfile> getEmulator(const QString &emulatorId) const;
    QList<EmulatorProfile> getAllEmulators() const;

    // Collections
    bool upsertCollection(const CollectionEntry &collection);
    bool removeCollection(const QString &collectionId);
    QList<CollectionEntry> getAllCollections() const;
    bool setGameInCollection(const QString &collectionId, const QString &gameId, bool included);

    // Statistics
    qint64 totalPlaytimeSeconds() const;
    int totalSessionsCount() const;

    // Safe atomic backup (SQLite VACUUM INTO or file copy after checkpoint)
    bool createBackup(const QString &destinationFile) const;

private:
    bool runMigrations();
    QSqlDatabase db() const;

    QString m_dbPath;
    QString m_connectionName;
    mutable QString m_lastError;
};

} // namespace Aether
