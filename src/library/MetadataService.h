#pragma once

#include "core/AppPaths.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

namespace Aether {

struct LocalArtworkResult {
    QString coverPath;
    QString backgroundPath;
};

class MetadataService : public QObject {
    Q_OBJECT
public:
    explicit MetadataService(const AppPaths &paths, QObject *parent = nullptr);

    // Cleans ROM/executable filenames into readable game titles
    static QString cleanTitleFromFilename(const QString &filePath);

    // Detects adjacent cover/background artwork in the same folder or media subfolder
    static LocalArtworkResult discoverLocalArtwork(const QString &gameFilePath);

    // Copies a user-selected image into userData/artwork/covers or backgrounds
    QString copyCustomArtwork(const QString &sourcePath, const QString &gameId, bool isBackground) const;

    // Fetches cover + hero background via SteamGridDB v2 if user has configured an API key
    void fetchSteamGridDbArtwork(const QString &gameId, const QString &gameTitle, const QString &apiKey);

signals:
    void artworkFetched(const QString &gameId, const QString &coverPath, const QString &backgroundPath);
    void metadataStatus(const QString &message);

private:
    void fetchAssetUrl(const QString &gameId, int sgdbGameId, const QString &apiKey, bool isHero);
    void downloadImage(const QString &gameId, const QUrl &url, bool isHero);

    AppPaths m_paths;
    QNetworkAccessManager m_net;
};

} // namespace Aether
