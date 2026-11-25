#include "MetadataService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

namespace Aether {

MetadataService::MetadataService(const AppPaths &paths, QObject *parent)
    : QObject(parent)
    , m_paths(paths) {}

QString MetadataService::cleanTitleFromFilename(const QString &filePath) {
    QFileInfo fi(filePath);
    QString base = fi.completeBaseName();

    // Strip common ROM/release bracketed tags: (USA), (Rev 1), [0100ABCD12340000], [v1.2.0], etc.
    base.remove(QRegularExpression(QStringLiteral(R"(\[[^\]]*\])")));
    base.remove(QRegularExpression(QStringLiteral(R"(\([^)]*\))")));
    base.replace(QLatin1Char('_'), QLatin1Char(' '));
    base.replace(QLatin1Char('.'), QLatin1Char(' '));
    base.replace(QRegularExpression(QStringLiteral(R"(\s+)")), QStringLiteral(" "));
    base = base.trimmed();

    if (base.isEmpty()) {
        return fi.fileName();
    }
    return base;
}

LocalArtworkResult MetadataService::discoverLocalArtwork(const QString &gameFilePath) {
    LocalArtworkResult res;
    const QFileInfo fi(gameFilePath);
    const QDir dir = fi.absoluteDir();
    const QString base = fi.completeBaseName();
    const QStringList imgExts = {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("webp")};

    auto findFirstExisting = [&](const QStringList &stemCandidates) -> QString {
        for (const QString &stem : stemCandidates) {
            for (const QString &ext : imgExts) {
                const QString path = dir.filePath(stem + QLatin1Char('.') + ext);
                if (QFileInfo::exists(path)) {
                    return path;
                }
            }
        }
        return {};
    };

    res.coverPath = findFirstExisting({
        base + QStringLiteral("-cover"),
        base + QStringLiteral("_cover"),
        base + QStringLiteral("-boxart"),
        base,
        QStringLiteral("cover"),
        QStringLiteral("boxart"),
        QStringLiteral("folder")
    });

    res.backgroundPath = findFirstExisting({
        base + QStringLiteral("-background"),
        base + QStringLiteral("_background"),
        base + QStringLiteral("-hero"),
        base + QStringLiteral("-fanart"),
        QStringLiteral("background"),
        QStringLiteral("hero"),
        QStringLiteral("fanart"),
        QStringLiteral("backdrop")
    });

    return res;
}

QString MetadataService::copyCustomArtwork(const QString &sourcePath, const QString &gameId, bool isBackground) const {
    if (sourcePath.isEmpty()) return {};
    QString localPath = sourcePath;
    if (localPath.startsWith(QStringLiteral("file://"))) {
        localPath = QUrl(localPath).toLocalFile();
    }

    const QFileInfo src(localPath);
    if (!src.exists() || !src.isFile()) return {};

    m_paths.ensureDirectoriesExist();
    const QString ext = src.suffix().isEmpty() ? QStringLiteral("png") : src.suffix().toLower();
    const QString targetDir = isBackground ? m_paths.backgroundsDir() : m_paths.coversDir();
    const QString destPath = QDir(targetDir).filePath(AppPaths::sanitizeFilename(gameId) + QLatin1Char('.') + ext);

    if (QFileInfo(destPath).canonicalFilePath() == src.canonicalFilePath()) {
        return destPath;
    }

    QFile::remove(destPath);
    if (QFile::copy(localPath, destPath)) {
        return destPath;
    }
    return {};
}

void MetadataService::fetchSteamGridDbArtwork(const QString &gameId, const QString &gameTitle, const QString &apiKey) {
    if (apiKey.trimmed().isEmpty()) {
        emit metadataStatus(QStringLiteral("Add a free SteamGridDB API key in Settings -> Metadata to fetch online artwork automatically."));
        return;
    }

    const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(gameTitle.trimmed()));
    const QUrl searchUrl(QStringLiteral("https://www.steamgriddb.com/api/v2/search/autocomplete/%1").arg(encoded));
    QNetworkRequest req(searchUrl);
    req.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.trimmed().toUtf8());
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AetherLauncher/1.0"));

    emit metadataStatus(QStringLiteral("Searching artwork for \"%1\"...").arg(gameTitle));
    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, gameId, gameTitle, apiKey]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit metadataStatus(QStringLiteral("Artwork lookup failed: %1").arg(reply->errorString()));
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
        if (data.isEmpty()) {
            emit metadataStatus(QStringLiteral("No online artwork matches found for \"%1\".").arg(gameTitle));
            return;
        }
        const int sgdbId = data.first().toObject().value(QStringLiteral("id")).toInt(-1);
        if (sgdbId <= 0) return;

        fetchAssetUrl(gameId, sgdbId, apiKey, false);
        fetchAssetUrl(gameId, sgdbId, apiKey, true);
    });
}

void MetadataService::fetchAssetUrl(const QString &gameId, int sgdbGameId, const QString &apiKey, bool isHero) {
    const QString endpoint = isHero
        ? QStringLiteral("https://www.steamgriddb.com/api/v2/heroes/game/%1").arg(sgdbGameId)
        : QStringLiteral("https://www.steamgriddb.com/api/v2/grids/game/%1?dimensions=600x900").arg(sgdbGameId);

    QNetworkRequest req{QUrl(endpoint)};
    req.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.trimmed().toUtf8());
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AetherLauncher/1.0"));

    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, gameId, isHero]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
        if (data.isEmpty()) return;

        const QUrl url(data.first().toObject().value(QStringLiteral("url")).toString());
        if (url.isValid()) {
            downloadImage(gameId, url, isHero);
        }
    });
}

void MetadataService::downloadImage(const QString &gameId, const QUrl &url, bool isHero) {
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AetherLauncher/1.0"));

    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, gameId, url, isHero]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        m_paths.ensureDirectoriesExist();
        QString ext = QFileInfo(url.path()).suffix().toLower();
        if (ext.isEmpty() || ext.size() > 5) ext = QStringLiteral("png");

        const QString dir = isHero ? m_paths.backgroundsDir() : m_paths.coversDir();
        const QString dest = QDir(dir).filePath(AppPaths::sanitizeFilename(gameId) + QLatin1Char('.') + ext);

        QFile f(dest);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(reply->readAll());
            f.close();
            if (isHero) {
                emit artworkFetched(gameId, QString(), dest);
            } else {
                emit artworkFetched(gameId, dest, QString());
            }
            emit metadataStatus(QStringLiteral("Artwork downloaded successfully."));
        }
    });
}

} // namespace Aether
