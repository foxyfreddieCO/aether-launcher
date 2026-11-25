#include "LibraryManager.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUrl>
#include <QUuid>
#include <algorithm>

namespace Aether {

namespace {
const QStringList kSections = {
    QStringLiteral("all"),
    QStringLiteral("favorites"),
    QStringLiteral("recent"),
    QStringLiteral("added"),
    QStringLiteral("most_played")
};
} // namespace

LibraryManager::LibraryManager(
    const AppPaths &paths,
    Database &database,
    EmulatorManager &emulatorManager,
    SettingsManager &settingsManager,
    QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_db(database)
    , m_emulators(emulatorManager)
    , m_settings(settingsManager)
    , m_metadata(paths, this) {
    connect(&m_metadata, &MetadataService::artworkFetched, this,
            [this](const QString &gameId, const QString &coverPath, const QString &backgroundPath) {
                QVariantMap fields;
                if (!coverPath.isEmpty()) fields.insert(QStringLiteral("coverPath"), coverPath);
                if (!backgroundPath.isEmpty()) fields.insert(QStringLiteral("backgroundPath"), backgroundPath);
                if (!fields.isEmpty()) {
                    updateGame(gameId, fields);
                }
            });
    connect(&m_metadata, &MetadataService::metadataStatus, this, &LibraryManager::setStatus);
}

void LibraryManager::reload() {
    m_games = m_db.getAllGames();
    m_collections = m_db.getAllCollections();
    emit gamesChanged();
    emit collectionsChanged();
}

void LibraryManager::setActiveSection(const QString &section) {
    if (m_activeSection == section) return;
    m_activeSection = section;
    emit filterChanged();
    emit gamesChanged();
}

void LibraryManager::setPlatformFilter(const QString &platform) {
    if (m_platformFilter == platform) return;
    m_platformFilter = platform;
    emit filterChanged();
    emit gamesChanged();
}

void LibraryManager::setCategoryFilter(const QString &category) {
    if (m_categoryFilter == category) return;
    m_categoryFilter = category;
    emit filterChanged();
    emit gamesChanged();
}

void LibraryManager::setCollectionFilter(const QString &collectionId) {
    if (m_collectionFilter == collectionId) return;
    m_collectionFilter = collectionId;
    emit filterChanged();
    emit gamesChanged();
}

void LibraryManager::setSearchQuery(const QString &query) {
    if (m_searchQuery == query) return;
    m_searchQuery = query;
    emit filterChanged();
    emit gamesChanged();
}

void LibraryManager::cycleSection(int delta) {
    int idx = kSections.indexOf(m_activeSection);
    if (idx < 0) idx = 0;
    idx = (idx + delta + kSections.size()) % kSections.size();
    setActiveSection(kSections.at(idx));
}

void LibraryManager::cycleCollection(int delta) {
    // Index 0 = All Collections (""), 1..N = user collections
    QStringList ids = {QString()};
    for (const auto &c : m_collections) ids.append(c.id);
    if (ids.size() <= 1) return;

    int idx = ids.indexOf(m_collectionFilter);
    if (idx < 0) idx = 0;
    idx = (idx + delta + ids.size()) % ids.size();
    setCollectionFilter(ids.at(idx));
}

QVariantList LibraryManager::filteredGames() const {
    QSet<QString> collectionMemberIds;
    bool useCollectionFilter = !m_collectionFilter.isEmpty();
    if (useCollectionFilter) {
        for (const auto &c : m_collections) {
            if (c.id == m_collectionFilter) {
                for (const QString &gid : c.gameIds) collectionMemberIds.insert(gid);
                break;
            }
        }
    }

    QList<GameEntry> list;
    const QString query = m_searchQuery.trimmed().toLower();

    for (const auto &g : m_games) {
        if (m_activeSection == QLatin1String("favorites") && !g.isFavorite) continue;
        if (m_activeSection == QLatin1String("recent") && !g.lastPlayed.isValid()) continue;
        if (m_activeSection == QLatin1String("most_played") && g.playtimeSeconds <= 0) continue;

        if (!m_platformFilter.isEmpty() && g.platform.compare(m_platformFilter, Qt::CaseInsensitive) != 0) continue;
        if (!m_categoryFilter.isEmpty() && g.category.compare(m_categoryFilter, Qt::CaseInsensitive) != 0) continue;
        if (useCollectionFilter && !collectionMemberIds.contains(g.id)) continue;

        if (!query.isEmpty()) {
            const bool matched = g.title.toLower().contains(query)
                || g.platform.toLower().contains(query)
                || g.category.toLower().contains(query)
                || g.developer.toLower().contains(query)
                || g.description.toLower().contains(query);
            if (!matched) continue;
        }
        list.append(g);
    }

    if (m_activeSection == QLatin1String("recent")) {
        std::sort(list.begin(), list.end(), [](const GameEntry &a, const GameEntry &b) {
            return a.lastPlayed > b.lastPlayed;
        });
    } else if (m_activeSection == QLatin1String("added")) {
        std::sort(list.begin(), list.end(), [](const GameEntry &a, const GameEntry &b) {
            return a.dateAdded > b.dateAdded;
        });
    } else if (m_activeSection == QLatin1String("most_played")) {
        std::sort(list.begin(), list.end(), [](const GameEntry &a, const GameEntry &b) {
            return a.playtimeSeconds > b.playtimeSeconds;
        });
    } else {
        std::sort(list.begin(), list.end(), [](const GameEntry &a, const GameEntry &b) {
            return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
        });
    }

    QVariantList out;
    out.reserve(list.size());
    for (const auto &g : list) {
        QVariantMap map = g.toVariantMap();
        map.insert(QStringLiteral("formattedPlaytime"), formatPlaytime(g.playtimeSeconds));
        map.insert(QStringLiteral("formattedLastPlayed"), formatDate(g.lastPlayed.isValid() ? g.lastPlayed.toString(Qt::ISODate) : QString()));
        out.append(map);
    }
    return out;
}

QVariantList LibraryManager::allGamesVariant() const {
    QVariantList out;
    for (const auto &g : m_games) {
        QVariantMap map = g.toVariantMap();
        map.insert(QStringLiteral("formattedPlaytime"), formatPlaytime(g.playtimeSeconds));
        map.insert(QStringLiteral("formattedLastPlayed"), formatDate(g.lastPlayed.isValid() ? g.lastPlayed.toString(Qt::ISODate) : QString()));
        out.append(map);
    }
    return out;
}

QVariantList LibraryManager::recentGamesVariant() const {
    QList<GameEntry> played;
    for (const auto &g : m_games) {
        if (g.lastPlayed.isValid()) played.append(g);
    }
    std::sort(played.begin(), played.end(), [](const GameEntry &a, const GameEntry &b) {
        return a.lastPlayed > b.lastPlayed;
    });

    QVariantList out;
    for (int i = 0; i < qMin(5, static_cast<int>(played.size())); ++i) {
        QVariantMap map = played.at(i).toVariantMap();
        map.insert(QStringLiteral("formattedPlaytime"), formatPlaytime(played.at(i).playtimeSeconds));
        out.append(map);
    }
    return out;
}

QVariantList LibraryManager::collectionsVariant() const {
    QVariantList out;
    for (const auto &c : m_collections) {
        out.append(c.toVariantMap());
    }
    return out;
}

QStringList LibraryManager::availablePlatforms() const {
    QSet<QString> set;
    for (const auto &g : m_games) {
        if (!g.platform.trimmed().isEmpty()) set.insert(g.platform.trimmed());
    }
    QStringList list(set.begin(), set.end());
    std::sort(list.begin(), list.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return list;
}

QStringList LibraryManager::availableCategories() const {
    QSet<QString> set;
    for (const auto &g : m_games) {
        if (!g.category.trimmed().isEmpty()) set.insert(g.category.trimmed());
    }
    QStringList list(set.begin(), set.end());
    std::sort(list.begin(), list.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return list;
}

QString LibraryManager::mostPlayedTitle() const {
    qint64 best = 0;
    QString title;
    for (const auto &g : m_games) {
        if (g.playtimeSeconds > best) {
            best = g.playtimeSeconds;
            title = g.title;
        }
    }
    return title.isEmpty() ? QStringLiteral("None yet") : title;
}

QVariantMap LibraryManager::getGameById(const QString &gameId) const {
    for (const auto &g : m_games) {
        if (g.id == gameId) {
            QVariantMap map = g.toVariantMap();
            map.insert(QStringLiteral("formattedPlaytime"), formatPlaytime(g.playtimeSeconds));
            map.insert(QStringLiteral("formattedLastPlayed"), formatDate(g.lastPlayed.isValid() ? g.lastPlayed.toString(Qt::ISODate) : QString()));
            return map;
        }
    }
    return {};
}

QString LibraryManager::normalizeFileUrl(const QString &urlOrPath) {
    QString cleaned = urlOrPath.trimmed();
    if (cleaned.startsWith(QStringLiteral("file://"))) {
        cleaned = QUrl(cleaned).toLocalFile();
    }
    return QDir::cleanPath(cleaned);
}

bool LibraryManager::hasGameWithPath(const QString &normalizedPath) const {
    for (const auto &g : m_games) {
#ifdef Q_OS_WIN
        if (QDir::cleanPath(g.executablePath).compare(normalizedPath, Qt::CaseInsensitive) == 0) return true;
#else
        if (QDir::cleanPath(g.executablePath) == normalizedPath) return true;
#endif
    }
    return false;
}

QString LibraryManager::addManualExecutable(const QString &exePath, const QString &title) {
    const QString localPath = normalizeFileUrl(exePath);
    if (localPath.isEmpty()) return {};

    GameEntry g;
    g.id = QStringLiteral("game-") + QUuid::createUuid().toString(QUuid::Id128).left(12);
    g.executablePath = localPath;
    g.title = title.trimmed().isEmpty() ? MetadataService::cleanTitleFromFilename(localPath) : title.trimmed();
    g.platform = QStringLiteral("Windows");
    g.category = QStringLiteral("PC Games");
    g.dateAdded = QDateTime::currentDateTimeUtc();

    const LocalArtworkResult art = MetadataService::discoverLocalArtwork(localPath);
    if (!art.coverPath.isEmpty()) g.coverPath = m_metadata.copyCustomArtwork(art.coverPath, g.id, false);
    if (!art.backgroundPath.isEmpty()) g.backgroundPath = m_metadata.copyCustomArtwork(art.backgroundPath, g.id, true);

    if (!m_db.upsertGame(g)) return {};
    reload();
    setStatus(QStringLiteral("Added \"%1\" to your library.").arg(g.title));

    if (g.coverPath.isEmpty() && !m_settings.steamGridDbApiKey().isEmpty()) {
        m_metadata.fetchSteamGridDbArtwork(g.id, g.title, m_settings.steamGridDbApiKey());
    }
    return g.id;
}

QString LibraryManager::addGameFile(const QString &filePath, const QString &emulatorId, const QString &title) {
    const QString localPath = normalizeFileUrl(filePath);
    if (localPath.isEmpty()) return {};

    QString resolvedEmuId = emulatorId;
    QString resolvedPlatform = QStringLiteral("Windows");

    if (resolvedEmuId.isEmpty()) {
        const auto matched = m_emulators.matchExtension(localPath);
        if (matched.has_value()) {
            resolvedEmuId = matched->id;
            resolvedPlatform = matched->platform;
        }
    } else {
        const auto emu = m_emulators.findById(resolvedEmuId);
        if (emu.has_value()) {
            resolvedPlatform = emu->platform;
        }
    }

    GameEntry g;
    g.id = QStringLiteral("game-") + QUuid::createUuid().toString(QUuid::Id128).left(12);
    g.executablePath = localPath;
    g.emulatorId = resolvedEmuId;
    g.title = title.trimmed().isEmpty() ? MetadataService::cleanTitleFromFilename(localPath) : title.trimmed();
    g.platform = resolvedPlatform;
    g.category = resolvedEmuId.isEmpty() ? QStringLiteral("PC Games") : QStringLiteral("Emulated");
    g.dateAdded = QDateTime::currentDateTimeUtc();

    const LocalArtworkResult art = MetadataService::discoverLocalArtwork(localPath);
    if (!art.coverPath.isEmpty()) g.coverPath = m_metadata.copyCustomArtwork(art.coverPath, g.id, false);
    if (!art.backgroundPath.isEmpty()) g.backgroundPath = m_metadata.copyCustomArtwork(art.backgroundPath, g.id, true);

    if (!m_db.upsertGame(g)) return {};
    reload();
    setStatus(QStringLiteral("Added \"%1\" (%2).").arg(g.title, g.platform));

    if (g.coverPath.isEmpty() && !m_settings.steamGridDbApiKey().isEmpty()) {
        m_metadata.fetchSteamGridDbArtwork(g.id, g.title, m_settings.steamGridDbApiKey());
    }
    return g.id;
}

bool LibraryManager::updateGame(const QString &gameId, const QVariantMap &fields) {
    auto existing = m_db.getGame(gameId);
    if (!existing.has_value()) return false;

    GameEntry g = *existing;
    if (fields.contains(QStringLiteral("title"))) g.title = fields.value(QStringLiteral("title")).toString().trimmed();
    if (fields.contains(QStringLiteral("platform"))) g.platform = fields.value(QStringLiteral("platform")).toString().trimmed();
    if (fields.contains(QStringLiteral("category"))) g.category = fields.value(QStringLiteral("category")).toString().trimmed();
    if (fields.contains(QStringLiteral("description"))) g.description = fields.value(QStringLiteral("description")).toString();
    if (fields.contains(QStringLiteral("executablePath"))) g.executablePath = normalizeFileUrl(fields.value(QStringLiteral("executablePath")).toString());
    if (fields.contains(QStringLiteral("workingDirectory"))) g.workingDirectory = normalizeFileUrl(fields.value(QStringLiteral("workingDirectory")).toString());
    if (fields.contains(QStringLiteral("emulatorId"))) g.emulatorId = fields.value(QStringLiteral("emulatorId")).toString();
    if (fields.contains(QStringLiteral("customArguments"))) g.customArguments = fields.value(QStringLiteral("customArguments")).toString();
    if (fields.contains(QStringLiteral("coverPath"))) g.coverPath = fields.value(QStringLiteral("coverPath")).toString();
    if (fields.contains(QStringLiteral("backgroundPath"))) g.backgroundPath = fields.value(QStringLiteral("backgroundPath")).toString();
    if (fields.contains(QStringLiteral("developer"))) g.developer = fields.value(QStringLiteral("developer")).toString().trimmed();
    if (fields.contains(QStringLiteral("releaseYear"))) g.releaseYear = fields.value(QStringLiteral("releaseYear")).toString().trimmed();
    if (fields.contains(QStringLiteral("isFavorite"))) g.isFavorite = fields.value(QStringLiteral("isFavorite")).toBool();

    if (!m_db.upsertGame(g)) return false;
    reload();
    return true;
}

bool LibraryManager::removeGame(const QString &gameId) {
    if (!m_db.removeGame(gameId)) return false;
    reload();
    setStatus(QStringLiteral("Game removed from library."));
    return true;
}

bool LibraryManager::toggleFavorite(const QString &gameId) {
    auto existing = m_db.getGame(gameId);
    if (!existing.has_value()) return false;
    existing->isFavorite = !existing->isFavorite;
    if (!m_db.upsertGame(*existing)) return false;
    reload();
    return true;
}

bool LibraryManager::setCustomCover(const QString &gameId, const QString &imagePath) {
    const QString copied = m_metadata.copyCustomArtwork(imagePath, gameId, false);
    if (copied.isEmpty()) return false;
    return updateGame(gameId, {{QStringLiteral("coverPath"), copied}});
}

bool LibraryManager::setCustomBackground(const QString &gameId, const QString &imagePath) {
    const QString copied = m_metadata.copyCustomArtwork(imagePath, gameId, true);
    if (copied.isEmpty()) return false;
    return updateGame(gameId, {{QStringLiteral("backgroundPath"), copied}});
}

void LibraryManager::fetchOnlineArtwork(const QString &gameId) {
    const auto g = m_db.getGame(gameId);
    if (!g.has_value()) return;
    m_metadata.fetchSteamGridDbArtwork(g->id, g->title, m_settings.steamGridDbApiKey());
}

int LibraryManager::scanDirectory(const QString &directoryPath, const QString &forcedEmulatorId) {
    const QString dirPath = normalizeFileUrl(directoryPath);
    QDir root(dirPath);
    if (!root.exists()) {
        setStatus(QStringLiteral("Scan directory does not exist: %1").arg(dirPath));
        return 0;
    }

    std::optional<EmulatorProfile> forcedEmu;
    if (!forcedEmulatorId.isEmpty()) {
        forcedEmu = m_emulators.findById(forcedEmulatorId);
    }

    int addedCount = 0;
    QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = QDir::cleanPath(it.next());
        if (hasGameWithPath(file)) continue;

        const QFileInfo fi(file);
        const QString ext = QStringLiteral(".") + fi.suffix().toLower();
        if (ext == QLatin1String(".")) continue;

        if (forcedEmu.has_value()) {
            bool extMatches = forcedEmu->supportedExtensions.isEmpty();
            for (const QString &e : forcedEmu->supportedExtensions) {
                if (e.toLower() == ext) {
                    extMatches = true;
                    break;
                }
            }
            if (!extMatches) continue;

            if (!addGameFile(file, forcedEmu->id).isEmpty()) {
                ++addedCount;
            }
        } else {
            // Check emulator extensions first, then Windows .exe
            const auto matched = m_emulators.matchExtension(file);
            if (matched.has_value()) {
                if (!addGameFile(file, matched->id).isEmpty()) {
                    ++addedCount;
                }
            } else if (ext == QLatin1String(".exe")) {
                // Skip obvious helper/uninstaller binaries
                const QString lowerName = fi.fileName().toLower();
                if (lowerName.contains(QStringLiteral("unins"))
                    || lowerName.contains(QStringLiteral("crashhandler"))
                    || lowerName.contains(QStringLiteral("redist"))
                    || lowerName.contains(QStringLiteral("vc_redist"))
                    || lowerName.contains(QStringLiteral("dxsetup"))) {
                    continue;
                }
                if (!addManualExecutable(file).isEmpty()) {
                    ++addedCount;
                }
            }
        }
    }

    setStatus(QStringLiteral("Scan complete: %1 new game(s) added.").arg(addedCount));
    return addedCount;
}

int LibraryManager::scanEmulatorDirectories(const QString &emulatorId) {
    const auto emu = m_emulators.findById(emulatorId);
    if (!emu.has_value()) return 0;

    int total = 0;
    for (const QString &dir : emu->gameDirectories) {
        total += scanDirectory(dir, emu->id);
    }
    setStatus(QStringLiteral("Scanned %1 directories: %2 new game(s) found.").arg(emu->name).arg(total));
    return total;
}

int LibraryManager::scanAllConfiguredDirectories() {
    int total = 0;
    for (const auto &emu : m_emulators.emulators()) {
        for (const QString &dir : emu.gameDirectories) {
            total += scanDirectory(dir, emu.id);
        }
    }
    setStatus(QStringLiteral("Full library scan finished: %1 new game(s) imported.").arg(total));
    return total;
}

QString LibraryManager::createCollection(const QString &name, const QString &description, const QString &accentColor) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) return {};

    CollectionEntry c;
    c.id = QStringLiteral("col-") + QUuid::createUuid().toString(QUuid::Id128).left(10);
    c.name = trimmed;
    c.description = description.trimmed();
    c.accentColor = accentColor.isEmpty() ? QStringLiteral("#4CC2FF") : accentColor;
    c.createdAt = QDateTime::currentDateTimeUtc();

    if (!m_db.upsertCollection(c)) return {};
    reload();
    return c.id;
}

bool LibraryManager::removeCollection(const QString &collectionId) {
    if (!m_db.removeCollection(collectionId)) return false;
    if (m_collectionFilter == collectionId) {
        m_collectionFilter.clear();
        emit filterChanged();
    }
    reload();
    return true;
}

bool LibraryManager::setGameInCollection(const QString &collectionId, const QString &gameId, bool included) {
    if (!m_db.setGameInCollection(collectionId, gameId, included)) return false;
    reload();
    return true;
}

bool LibraryManager::isGameInCollection(const QString &collectionId, const QString &gameId) const {
    for (const auto &c : m_collections) {
        if (c.id == collectionId) {
            return c.gameIds.contains(gameId);
        }
    }
    return false;
}

QString LibraryManager::createBackupSnapshot() {
    m_paths.ensureDirectoriesExist();
    const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString backupDir = QDir(m_paths.backupsDir()).filePath(QStringLiteral("backup-") + stamp);
    QDir().mkpath(backupDir);

    const QString dbDest = QDir(backupDir).filePath(QStringLiteral("library.sqlite"));
    const bool dbOk = m_db.createBackup(dbDest);
    const QString settingsDest = QDir(backupDir).filePath(QStringLiteral("settings.json"));
    QFile::remove(settingsDest);
    QFile::copy(m_paths.settingsFile(), settingsDest);

    pruneOldBackups(10);
    if (dbOk) {
        setStatus(QStringLiteral("Library backup saved (%1).").arg(stamp));
        return backupDir;
    }
    return {};
}

QVariantList LibraryManager::listBackups() const {
    QDir dir(m_paths.backupsDir());
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    QVariantList out;
    for (const auto &fi : entries) {
        out.append(QVariantMap{
            {QStringLiteral("name"), fi.fileName()},
            {QStringLiteral("path"), fi.absoluteFilePath()},
            {QStringLiteral("modified"), fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"))}
        });
    }
    return out;
}

void LibraryManager::pruneOldBackups(int maxKeep) const {
    QDir dir(m_paths.backupsDir());
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (int i = maxKeep; i < entries.size(); ++i) {
        QDir(entries.at(i).absoluteFilePath()).removeRecursively();
    }
}

bool LibraryManager::exportConfigurationBundle(const QString &filePath) const {
    const QString localPath = normalizeFileUrl(filePath);
    if (localPath.isEmpty()) return false;

    QJsonObject bundle;
    bundle.insert(QStringLiteral("format"), QStringLiteral("aether-launcher-bundle-v1"));
    bundle.insert(QStringLiteral("exportedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    bundle.insert(QStringLiteral("settings"), m_settings.exportToJson());

    QJsonArray emus;
    for (const auto &e : m_emulators.emulators()) emus.append(e.toJson());
    bundle.insert(QStringLiteral("emulators"), emus);

    QJsonArray games;
    for (const auto &g : m_games) games.append(g.toJson());
    bundle.insert(QStringLiteral("games"), games);

    QJsonArray cols;
    for (const auto &c : m_collections) cols.append(c.toJson());
    bundle.insert(QStringLiteral("collections"), cols);

    QFile f(localPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(bundle).toJson(QJsonDocument::Indented));
    return true;
}

bool LibraryManager::importConfigurationBundle(const QString &filePath) {
    const QString localPath = normalizeFileUrl(filePath);
    QFile f(localPath);
    if (!f.open(QIODevice::ReadOnly)) return false;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;
    const QJsonObject bundle = doc.object();

    if (bundle.value(QStringLiteral("settings")).isObject()) {
        m_settings.importFromJson(bundle.value(QStringLiteral("settings")).toObject());
        m_settings.save();
    }

    for (const auto &v : bundle.value(QStringLiteral("emulators")).toArray()) {
        if (v.isObject()) m_db.upsertEmulator(EmulatorProfile::fromJson(v.toObject()));
    }
    for (const auto &v : bundle.value(QStringLiteral("games")).toArray()) {
        if (v.isObject()) m_db.upsertGame(GameEntry::fromJson(v.toObject()));
    }
    for (const auto &v : bundle.value(QStringLiteral("collections")).toArray()) {
        if (v.isObject()) m_db.upsertCollection(CollectionEntry::fromJson(v.toObject()));
    }

    m_emulators.reload();
    reload();
    setStatus(QStringLiteral("Configuration bundle imported successfully."));
    return true;
}

QString LibraryManager::formatPlaytime(qint64 seconds) {
    if (seconds <= 0) return QStringLiteral("Never played");
    if (seconds < 60) return QStringLiteral("%1s").arg(seconds);
    const qint64 mins = seconds / 60;
    if (mins < 60) return QStringLiteral("%1m").arg(mins);
    const qint64 hours = mins / 60;
    const qint64 remMins = mins % 60;
    if (remMins == 0) return QStringLiteral("%1h").arg(hours);
    return QStringLiteral("%1h %2m").arg(hours).arg(remMins);
}

QString LibraryManager::formatDate(const QString &isoDate) {
    if (isoDate.isEmpty()) return QStringLiteral("Never");
    const QDateTime dt = QDateTime::fromString(isoDate, Qt::ISODate).toLocalTime();
    if (!dt.isValid()) return QStringLiteral("Never");
    return dt.toString(QStringLiteral("MMM d, yyyy"));
}

void LibraryManager::setStatus(const QString &message) {
    m_lastStatusMessage = message;
    emit statusMessageChanged(message);
}

} // namespace Aether
