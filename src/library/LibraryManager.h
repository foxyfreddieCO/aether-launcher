#pragma once

#include "MetadataService.h"
#include "core/AppPaths.h"
#include "core/Database.h"
#include "core/Models.h"
#include "core/SettingsManager.h"
#include "emulators/EmulatorManager.h"

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace Aether {

class LibraryManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList filteredGames READ filteredGames NOTIFY gamesChanged)
    Q_PROPERTY(QVariantList allGames READ allGamesVariant NOTIFY gamesChanged)
    Q_PROPERTY(QVariantList recentGames READ recentGamesVariant NOTIFY gamesChanged)
    Q_PROPERTY(QVariantList collections READ collectionsVariant NOTIFY collectionsChanged)
    Q_PROPERTY(QStringList availablePlatforms READ availablePlatforms NOTIFY gamesChanged)
    Q_PROPERTY(QStringList availableCategories READ availableCategories NOTIFY gamesChanged)

    Q_PROPERTY(QString activeSection READ activeSection WRITE setActiveSection NOTIFY filterChanged)
    Q_PROPERTY(QString platformFilter READ platformFilter WRITE setPlatformFilter NOTIFY filterChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY filterChanged)
    Q_PROPERTY(QString collectionFilter READ collectionFilter WRITE setCollectionFilter NOTIFY filterChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY filterChanged)

    Q_PROPERTY(int totalGamesCount READ totalGamesCount NOTIFY gamesChanged)
    Q_PROPERTY(qint64 totalPlaytimeSeconds READ totalPlaytimeSeconds NOTIFY gamesChanged)
    Q_PROPERTY(int totalSessionsCount READ totalSessionsCount NOTIFY gamesChanged)
    Q_PROPERTY(QString mostPlayedTitle READ mostPlayedTitle NOTIFY gamesChanged)
    Q_PROPERTY(QString lastStatusMessage READ lastStatusMessage NOTIFY statusMessageChanged)

public:
    explicit LibraryManager(
        const AppPaths &paths,
        Database &database,
        EmulatorManager &emulatorManager,
        SettingsManager &settingsManager,
        QObject *parent = nullptr);

    void reload();

    // Filters
    QString activeSection() const { return m_activeSection; }
    void setActiveSection(const QString &section);
    QString platformFilter() const { return m_platformFilter; }
    void setPlatformFilter(const QString &platform);
    QString categoryFilter() const { return m_categoryFilter; }
    void setCategoryFilter(const QString &category);
    QString collectionFilter() const { return m_collectionFilter; }
    void setCollectionFilter(const QString &collectionId);
    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    Q_INVOKABLE void cycleSection(int delta);
    Q_INVOKABLE void cycleCollection(int delta);

    // Data lists
    QVariantList filteredGames() const;
    QVariantList allGamesVariant() const;
    QVariantList recentGamesVariant() const;
    QVariantList collectionsVariant() const;
    QStringList availablePlatforms() const;
    QStringList availableCategories() const;

    // Stats
    int totalGamesCount() const { return m_games.size(); }
    qint64 totalPlaytimeSeconds() const { return m_db.totalPlaytimeSeconds(); }
    int totalSessionsCount() const { return m_db.totalSessionsCount(); }
    QString mostPlayedTitle() const;
    QString lastStatusMessage() const { return m_lastStatusMessage; }

    // Game CRUD
    Q_INVOKABLE QVariantMap getGameById(const QString &gameId) const;
    Q_INVOKABLE QString addManualExecutable(const QString &exePath, const QString &title = QString());
    Q_INVOKABLE QString addGameFile(const QString &filePath, const QString &emulatorId = QString(), const QString &title = QString());
    Q_INVOKABLE bool updateGame(const QString &gameId, const QVariantMap &fields);
    Q_INVOKABLE bool removeGame(const QString &gameId);
    Q_INVOKABLE bool toggleFavorite(const QString &gameId);

    // Artwork & Metadata
    Q_INVOKABLE bool setCustomCover(const QString &gameId, const QString &imagePath);
    Q_INVOKABLE bool setCustomBackground(const QString &gameId, const QString &imagePath);
    Q_INVOKABLE void fetchOnlineArtwork(const QString &gameId);

    // Scanning
    Q_INVOKABLE int scanDirectory(const QString &directoryPath, const QString &forcedEmulatorId = QString());
    Q_INVOKABLE int scanEmulatorDirectories(const QString &emulatorId);
    Q_INVOKABLE int scanAllConfiguredDirectories();

    // Collections
    Q_INVOKABLE QString createCollection(const QString &name, const QString &description = QString(), const QString &accentColor = QStringLiteral("#4CC2FF"));
    Q_INVOKABLE bool removeCollection(const QString &collectionId);
    Q_INVOKABLE bool setGameInCollection(const QString &collectionId, const QString &gameId, bool included);
    Q_INVOKABLE bool isGameInCollection(const QString &collectionId, const QString &gameId) const;

    // Backups & Import/Export
    Q_INVOKABLE QString createBackupSnapshot();
    Q_INVOKABLE QVariantList listBackups() const;
    Q_INVOKABLE bool exportConfigurationBundle(const QString &filePath) const;
    Q_INVOKABLE bool importConfigurationBundle(const QString &filePath);

    // Utility
    Q_INVOKABLE static QString formatPlaytime(qint64 seconds);
    Q_INVOKABLE static QString formatDate(const QString &isoDate);
    Q_INVOKABLE static QString normalizeFileUrl(const QString &urlOrPath);

signals:
    void gamesChanged();
    void collectionsChanged();
    void filterChanged();
    void statusMessageChanged(const QString &message);

private:
    void setStatus(const QString &message);
    bool hasGameWithPath(const QString &normalizedPath) const;
    void pruneOldBackups(int maxKeep = 10) const;

    AppPaths m_paths;
    Database &m_db;
    EmulatorManager &m_emulators;
    SettingsManager &m_settings;
    MetadataService m_metadata;

    QList<GameEntry> m_games;
    QList<CollectionEntry> m_collections;

    // Sections: all, favorites, recent, added, most_played
    QString m_activeSection = QStringLiteral("all");
    QString m_platformFilter;
    QString m_categoryFilter;
    QString m_collectionFilter;
    QString m_searchQuery;
    QString m_lastStatusMessage;
};

} // namespace Aether
