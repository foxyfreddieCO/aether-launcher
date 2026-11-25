#include "Database.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace Aether {

namespace {
QString serializeStringList(const QStringList &list) {
    QJsonArray arr;
    for (const auto &s : list) arr.append(s);
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QStringList deserializeStringList(const QString &json) {
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QStringList out;
    if (doc.isArray()) {
        for (const auto &v : doc.array()) {
            if (v.isString()) out.append(v.toString());
        }
    }
    return out;
}

GameEntry gameFromQuery(const QSqlQuery &q) {
    GameEntry g;
    g.id = q.value(0).toString();
    g.title = q.value(1).toString();
    g.platform = q.value(2).toString();
    g.category = q.value(3).toString();
    g.description = q.value(4).toString();
    g.executablePath = q.value(5).toString();
    g.workingDirectory = q.value(6).toString();
    g.emulatorId = q.value(7).toString();
    g.customArguments = q.value(8).toString();
    g.coverPath = q.value(9).toString();
    g.backgroundPath = q.value(10).toString();
    g.developer = q.value(11).toString();
    g.releaseYear = q.value(12).toString();
    g.isFavorite = q.value(13).toInt() != 0;
    g.playtimeSeconds = q.value(14).toLongLong();
    g.playCount = q.value(15).toInt();
    const QString lp = q.value(16).toString();
    if (!lp.isEmpty()) g.lastPlayed = QDateTime::fromString(lp, Qt::ISODate);
    const QString da = q.value(17).toString();
    if (!da.isEmpty()) g.dateAdded = QDateTime::fromString(da, Qt::ISODate);
    return g;
}

EmulatorProfile emulatorFromQuery(const QSqlQuery &q) {
    EmulatorProfile e;
    e.id = q.value(0).toString();
    e.name = q.value(1).toString();
    e.platform = q.value(2).toString();
    e.executablePath = q.value(3).toString();
    e.launchArguments = q.value(4).toString();
    e.workingDirectory = q.value(5).toString();
    e.supportedExtensions = deserializeStringList(q.value(6).toString());
    e.gameDirectories = deserializeStringList(q.value(7).toString());
    e.iconName = q.value(8).toString();
    e.isDedicatedLichii = q.value(9).toInt() != 0;
    e.notes = q.value(10).toString();
    return e;
}
} // namespace

Database::Database(const QString &dbPath)
    : m_dbPath(dbPath)
    , m_connectionName(QStringLiteral("aether_db_") + QUuid::createUuid().toString(QUuid::Id128)) {}

Database::~Database() {
    close();
}

QSqlDatabase Database::db() const {
    return QSqlDatabase::database(m_connectionName);
}

bool Database::open() {
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());

    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_dbPath);
    if (!database.open()) {
        m_lastError = database.lastError().text();
        return false;
    }

    QSqlQuery pragma(database);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
    pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));
    pragma.exec(QStringLiteral("PRAGMA synchronous = NORMAL;"));

    return runMigrations();
}

void Database::close() {
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase database = QSqlDatabase::database(m_connectionName, false);
            if (database.isOpen()) {
                database.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool Database::isOpen() const {
    if (!QSqlDatabase::contains(m_connectionName)) return false;
    return QSqlDatabase::database(m_connectionName, false).isOpen();
}

bool Database::runMigrations() {
    QSqlQuery q(db());

    const QStringList statements = {
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS schema_meta (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            );
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS emulators (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                platform TEXT NOT NULL,
                executable_path TEXT NOT NULL DEFAULT '',
                launch_arguments TEXT NOT NULL DEFAULT '"{rom}"',
                working_directory TEXT NOT NULL DEFAULT '',
                supported_extensions TEXT NOT NULL DEFAULT '[]',
                game_directories TEXT NOT NULL DEFAULT '[]',
                icon_name TEXT NOT NULL DEFAULT 'gamepad',
                is_dedicated_lichii INTEGER NOT NULL DEFAULT 0,
                notes TEXT NOT NULL DEFAULT ''
            );
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS games (
                id TEXT PRIMARY KEY,
                title TEXT NOT NULL,
                platform TEXT NOT NULL DEFAULT 'Windows',
                category TEXT NOT NULL DEFAULT 'Uncategorized',
                description TEXT NOT NULL DEFAULT '',
                executable_path TEXT NOT NULL,
                working_directory TEXT NOT NULL DEFAULT '',
                emulator_id TEXT NOT NULL DEFAULT '',
                custom_arguments TEXT NOT NULL DEFAULT '',
                cover_path TEXT NOT NULL DEFAULT '',
                background_path TEXT NOT NULL DEFAULT '',
                developer TEXT NOT NULL DEFAULT '',
                release_year TEXT NOT NULL DEFAULT '',
                is_favorite INTEGER NOT NULL DEFAULT 0,
                playtime_seconds INTEGER NOT NULL DEFAULT 0,
                play_count INTEGER NOT NULL DEFAULT 0,
                last_played TEXT NOT NULL DEFAULT '',
                date_added TEXT NOT NULL DEFAULT ''
            );
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS collections (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT NOT NULL DEFAULT '',
                icon_name TEXT NOT NULL DEFAULT 'collection',
                accent_color TEXT NOT NULL DEFAULT '#4CC2FF',
                created_at TEXT NOT NULL DEFAULT ''
            );
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS collection_games (
                collection_id TEXT NOT NULL,
                game_id TEXT NOT NULL,
                PRIMARY KEY (collection_id, game_id),
                FOREIGN KEY (collection_id) REFERENCES collections(id) ON DELETE CASCADE,
                FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE
            );
        )"),
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS play_sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                game_id TEXT NOT NULL,
                duration_seconds INTEGER NOT NULL,
                started_at TEXT NOT NULL,
                ended_at TEXT NOT NULL,
                FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE
            );
        )"),
        QStringLiteral("INSERT OR REPLACE INTO schema_meta (key, value) VALUES ('version', '1');")
    };

    for (const QString &sql : statements) {
        if (!q.exec(sql)) {
            m_lastError = q.lastError().text();
            return false;
        }
    }
    return true;
}

bool Database::upsertGame(const GameEntry &game) {
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO games (
            id, title, platform, category, description, executable_path,
            working_directory, emulator_id, custom_arguments, cover_path,
            background_path, developer, release_year, is_favorite,
            playtime_seconds, play_count, last_played, date_added
        ) VALUES (
            :id, :title, :platform, :category, :description, :executable_path,
            :working_directory, :emulator_id, :custom_arguments, :cover_path,
            :background_path, :developer, :release_year, :is_favorite,
            :playtime_seconds, :play_count, :last_played, :date_added
        );
    )"));
    q.bindValue(QStringLiteral(":id"), game.id);
    q.bindValue(QStringLiteral(":title"), game.title);
    q.bindValue(QStringLiteral(":platform"), game.platform);
    q.bindValue(QStringLiteral(":category"), game.category);
    q.bindValue(QStringLiteral(":description"), game.description);
    q.bindValue(QStringLiteral(":executable_path"), game.executablePath);
    q.bindValue(QStringLiteral(":working_directory"), game.workingDirectory);
    q.bindValue(QStringLiteral(":emulator_id"), game.emulatorId);
    q.bindValue(QStringLiteral(":custom_arguments"), game.customArguments);
    q.bindValue(QStringLiteral(":cover_path"), game.coverPath);
    q.bindValue(QStringLiteral(":background_path"), game.backgroundPath);
    q.bindValue(QStringLiteral(":developer"), game.developer);
    q.bindValue(QStringLiteral(":release_year"), game.releaseYear);
    q.bindValue(QStringLiteral(":is_favorite"), game.isFavorite ? 1 : 0);
    q.bindValue(QStringLiteral(":playtime_seconds"), game.playtimeSeconds);
    q.bindValue(QStringLiteral(":play_count"), game.playCount);
    q.bindValue(QStringLiteral(":last_played"), game.lastPlayed.isValid() ? game.lastPlayed.toString(Qt::ISODate) : QString());
    q.bindValue(QStringLiteral(":date_added"), game.dateAdded.isValid() ? game.dateAdded.toString(Qt::ISODate) : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::removeGame(const QString &gameId) {
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM games WHERE id = :id;"));
    q.bindValue(QStringLiteral(":id"), gameId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

std::optional<GameEntry> Database::getGame(const QString &gameId) const {
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        SELECT id, title, platform, category, description, executable_path,
               working_directory, emulator_id, custom_arguments, cover_path,
               background_path, developer, release_year, is_favorite,
               playtime_seconds, play_count, last_played, date_added
        FROM games WHERE id = :id;
    )"));
    q.bindValue(QStringLiteral(":id"), gameId);
    if (!q.exec() || !q.next()) {
        return std::nullopt;
    }
    return gameFromQuery(q);
}

QList<GameEntry> Database::getAllGames() const {
    QList<GameEntry> games;
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        SELECT id, title, platform, category, description, executable_path,
               working_directory, emulator_id, custom_arguments, cover_path,
               background_path, developer, release_year, is_favorite,
               playtime_seconds, play_count, last_played, date_added
        FROM games ORDER BY title COLLATE NOCASE ASC;
    )"));
    if (q.exec()) {
        while (q.next()) {
            games.append(gameFromQuery(q));
        }
    }
    return games;
}

bool Database::recordPlaySession(const QString &gameId, qint64 durationSeconds, const QDateTime &startedAt, const QDateTime &endedAt) {
    QSqlDatabase database = db();
    database.transaction();

    QSqlQuery ins(database);
    ins.prepare(QStringLiteral(R"(
        INSERT INTO play_sessions (game_id, duration_seconds, started_at, ended_at)
        VALUES (:game_id, :duration, :started_at, :ended_at);
    )"));
    ins.bindValue(QStringLiteral(":game_id"), gameId);
    ins.bindValue(QStringLiteral(":duration"), qMax<qint64>(0, durationSeconds));
    ins.bindValue(QStringLiteral(":started_at"), startedAt.toUTC().toString(Qt::ISODate));
    ins.bindValue(QStringLiteral(":ended_at"), endedAt.toUTC().toString(Qt::ISODate));

    QSqlQuery upd(database);
    upd.prepare(QStringLiteral(R"(
        UPDATE games
        SET playtime_seconds = playtime_seconds + :duration,
            play_count = play_count + 1,
            last_played = :ended_at
        WHERE id = :game_id;
    )"));
    upd.bindValue(QStringLiteral(":duration"), qMax<qint64>(0, durationSeconds));
    upd.bindValue(QStringLiteral(":ended_at"), endedAt.toUTC().toString(Qt::ISODate));
    upd.bindValue(QStringLiteral(":game_id"), gameId);

    if (!ins.exec() || !upd.exec()) {
        m_lastError = ins.lastError().isValid() ? ins.lastError().text() : upd.lastError().text();
        database.rollback();
        return false;
    }
    return database.commit();
}

bool Database::upsertEmulator(const EmulatorProfile &emulator) {
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO emulators (
            id, name, platform, executable_path, launch_arguments,
            working_directory, supported_extensions, game_directories,
            icon_name, is_dedicated_lichii, notes
        ) VALUES (
            :id, :name, :platform, :executable_path, :launch_arguments,
            :working_directory, :supported_extensions, :game_directories,
            :icon_name, :is_dedicated_lichii, :notes
        );
    )"));
    q.bindValue(QStringLiteral(":id"), emulator.id);
    q.bindValue(QStringLiteral(":name"), emulator.name);
    q.bindValue(QStringLiteral(":platform"), emulator.platform);
    q.bindValue(QStringLiteral(":executable_path"), emulator.executablePath);
    q.bindValue(QStringLiteral(":launch_arguments"), emulator.launchArguments);
    q.bindValue(QStringLiteral(":working_directory"), emulator.workingDirectory);
    q.bindValue(QStringLiteral(":supported_extensions"), serializeStringList(emulator.supportedExtensions));
    q.bindValue(QStringLiteral(":game_directories"), serializeStringList(emulator.gameDirectories));
    q.bindValue(QStringLiteral(":icon_name"), emulator.iconName);
    q.bindValue(QStringLiteral(":is_dedicated_lichii"), emulator.isDedicatedLichii ? 1 : 0);
    q.bindValue(QStringLiteral(":notes"), emulator.notes);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::removeEmulator(const QString &emulatorId) {
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM emulators WHERE id = :id;"));
    q.bindValue(QStringLiteral(":id"), emulatorId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

std::optional<EmulatorProfile> Database::getEmulator(const QString &emulatorId) const {
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        SELECT id, name, platform, executable_path, launch_arguments,
               working_directory, supported_extensions, game_directories,
               icon_name, is_dedicated_lichii, notes
        FROM emulators WHERE id = :id;
    )"));
    q.bindValue(QStringLiteral(":id"), emulatorId);
    if (!q.exec() || !q.next()) {
        return std::nullopt;
    }
    return emulatorFromQuery(q);
}

QList<EmulatorProfile> Database::getAllEmulators() const {
    QList<EmulatorProfile> list;
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        SELECT id, name, platform, executable_path, launch_arguments,
               working_directory, supported_extensions, game_directories,
               icon_name, is_dedicated_lichii, notes
        FROM emulators ORDER BY is_dedicated_lichii DESC, name COLLATE NOCASE ASC;
    )"));
    if (q.exec()) {
        while (q.next()) {
            list.append(emulatorFromQuery(q));
        }
    }
    return list;
}

bool Database::upsertCollection(const CollectionEntry &collection) {
    QSqlDatabase database = db();
    database.transaction();

    QSqlQuery q(database);
    q.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO collections (id, name, description, icon_name, accent_color, created_at)
        VALUES (:id, :name, :description, :icon_name, :accent_color, :created_at);
    )"));
    q.bindValue(QStringLiteral(":id"), collection.id);
    q.bindValue(QStringLiteral(":name"), collection.name);
    q.bindValue(QStringLiteral(":description"), collection.description);
    q.bindValue(QStringLiteral(":icon_name"), collection.iconName);
    q.bindValue(QStringLiteral(":accent_color"), collection.accentColor);
    q.bindValue(QStringLiteral(":created_at"), collection.createdAt.isValid()
                    ? collection.createdAt.toUTC().toString(Qt::ISODate)
                    : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        database.rollback();
        return false;
    }

    QSqlQuery clr(database);
    clr.prepare(QStringLiteral("DELETE FROM collection_games WHERE collection_id = :cid;"));
    clr.bindValue(QStringLiteral(":cid"), collection.id);
    if (!clr.exec()) {
        m_lastError = clr.lastError().text();
        database.rollback();
        return false;
    }

    QSqlQuery ins(database);
    ins.prepare(QStringLiteral("INSERT OR IGNORE INTO collection_games (collection_id, game_id) VALUES (:cid, :gid);"));
    for (const QString &gid : collection.gameIds) {
        ins.bindValue(QStringLiteral(":cid"), collection.id);
        ins.bindValue(QStringLiteral(":gid"), gid);
        ins.exec();
    }

    return database.commit();
}

bool Database::removeCollection(const QString &collectionId) {
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM collections WHERE id = :id;"));
    q.bindValue(QStringLiteral(":id"), collectionId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

QList<CollectionEntry> Database::getAllCollections() const {
    QList<CollectionEntry> result;
    QSqlQuery q(db());
    if (!q.exec(QStringLiteral("SELECT id, name, description, icon_name, accent_color, created_at FROM collections ORDER BY name COLLATE NOCASE ASC;"))) {
        return result;
    }
    while (q.next()) {
        CollectionEntry c;
        c.id = q.value(0).toString();
        c.name = q.value(1).toString();
        c.description = q.value(2).toString();
        c.iconName = q.value(3).toString();
        c.accentColor = q.value(4).toString();
        const QString ts = q.value(5).toString();
        if (!ts.isEmpty()) c.createdAt = QDateTime::fromString(ts, Qt::ISODate);

        QSqlQuery gq(db());
        gq.prepare(QStringLiteral("SELECT game_id FROM collection_games WHERE collection_id = :cid;"));
        gq.bindValue(QStringLiteral(":cid"), c.id);
        if (gq.exec()) {
            while (gq.next()) {
                c.gameIds.append(gq.value(0).toString());
            }
        }
        result.append(c);
    }
    return result;
}

bool Database::setGameInCollection(const QString &collectionId, const QString &gameId, bool included) {
    QSqlQuery q(db());
    if (included) {
        q.prepare(QStringLiteral("INSERT OR IGNORE INTO collection_games (collection_id, game_id) VALUES (:cid, :gid);"));
    } else {
        q.prepare(QStringLiteral("DELETE FROM collection_games WHERE collection_id = :cid AND game_id = :gid;"));
    }
    q.bindValue(QStringLiteral(":cid"), collectionId);
    q.bindValue(QStringLiteral(":gid"), gameId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

qint64 Database::totalPlaytimeSeconds() const {
    QSqlQuery q(db());
    if (q.exec(QStringLiteral("SELECT COALESCE(SUM(playtime_seconds), 0) FROM games;")) && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

int Database::totalSessionsCount() const {
    QSqlQuery q(db());
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM play_sessions;")) && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool Database::createBackup(const QString &destinationFile) const {
    QDir().mkpath(QFileInfo(destinationFile).absolutePath());
    QFile::remove(destinationFile);

    QSqlQuery q(db());
    q.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE);"));

    // VACUUM INTO creates an atomic, consistent snapshot without closing DB
    QSqlQuery vac(db());
    vac.prepare(QStringLiteral("VACUUM INTO :dest;"));
    vac.bindValue(QStringLiteral(":dest"), destinationFile);
    if (vac.exec()) {
        return true;
    }

    // Fallback: direct file copy if older SQLite build lacks VACUUM INTO
    return QFile::copy(m_dbPath, destinationFile);
}

} // namespace Aether
