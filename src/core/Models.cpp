#include "Models.h"

namespace Aether {

namespace {
QJsonArray toJsonArray(const QStringList &list) {
    QJsonArray arr;
    for (const auto &s : list) {
        arr.append(s);
    }
    return arr;
}

QStringList fromJsonArray(const QJsonArray &arr) {
    QStringList out;
    out.reserve(arr.size());
    for (const auto &v : arr) {
        if (v.isString()) {
            out.append(v.toString());
        }
    }
    return out;
}
} // namespace

QVariantMap GameEntry::toVariantMap() const {
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("platform"), platform},
        {QStringLiteral("category"), category},
        {QStringLiteral("description"), description},
        {QStringLiteral("executablePath"), executablePath},
        {QStringLiteral("workingDirectory"), workingDirectory},
        {QStringLiteral("emulatorId"), emulatorId},
        {QStringLiteral("customArguments"), customArguments},
        {QStringLiteral("coverPath"), coverPath},
        {QStringLiteral("backgroundPath"), backgroundPath},
        {QStringLiteral("developer"), developer},
        {QStringLiteral("releaseYear"), releaseYear},
        {QStringLiteral("isFavorite"), isFavorite},
        {QStringLiteral("playtimeSeconds"), playtimeSeconds},
        {QStringLiteral("playCount"), playCount},
        {QStringLiteral("lastPlayed"), lastPlayed.isValid() ? lastPlayed.toString(Qt::ISODate) : QString()},
        {QStringLiteral("dateAdded"), dateAdded.isValid() ? dateAdded.toString(Qt::ISODate) : QString()}
    };
}

QJsonObject GameEntry::toJson() const {
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("platform"), platform},
        {QStringLiteral("category"), category},
        {QStringLiteral("description"), description},
        {QStringLiteral("executablePath"), executablePath},
        {QStringLiteral("workingDirectory"), workingDirectory},
        {QStringLiteral("emulatorId"), emulatorId},
        {QStringLiteral("customArguments"), customArguments},
        {QStringLiteral("coverPath"), coverPath},
        {QStringLiteral("backgroundPath"), backgroundPath},
        {QStringLiteral("developer"), developer},
        {QStringLiteral("releaseYear"), releaseYear},
        {QStringLiteral("isFavorite"), isFavorite},
        {QStringLiteral("playtimeSeconds"), static_cast<qint64>(playtimeSeconds)},
        {QStringLiteral("playCount"), playCount},
        {QStringLiteral("lastPlayed"), lastPlayed.isValid() ? lastPlayed.toString(Qt::ISODate) : QString()},
        {QStringLiteral("dateAdded"), dateAdded.isValid() ? dateAdded.toString(Qt::ISODate) : QString()}
    };
}

GameEntry GameEntry::fromJson(const QJsonObject &obj) {
    GameEntry g;
    g.id = obj.value(QStringLiteral("id")).toString();
    g.title = obj.value(QStringLiteral("title")).toString();
    g.platform = obj.value(QStringLiteral("platform")).toString(QStringLiteral("Windows"));
    g.category = obj.value(QStringLiteral("category")).toString(QStringLiteral("Uncategorized"));
    g.description = obj.value(QStringLiteral("description")).toString();
    g.executablePath = obj.value(QStringLiteral("executablePath")).toString();
    g.workingDirectory = obj.value(QStringLiteral("workingDirectory")).toString();
    g.emulatorId = obj.value(QStringLiteral("emulatorId")).toString();
    g.customArguments = obj.value(QStringLiteral("customArguments")).toString();
    g.coverPath = obj.value(QStringLiteral("coverPath")).toString();
    g.backgroundPath = obj.value(QStringLiteral("backgroundPath")).toString();
    g.developer = obj.value(QStringLiteral("developer")).toString();
    g.releaseYear = obj.value(QStringLiteral("releaseYear")).toString();
    g.isFavorite = obj.value(QStringLiteral("isFavorite")).toBool(false);
    g.playtimeSeconds = static_cast<qint64>(obj.value(QStringLiteral("playtimeSeconds")).toDouble(0));
    g.playCount = obj.value(QStringLiteral("playCount")).toInt(0);
    const QString lp = obj.value(QStringLiteral("lastPlayed")).toString();
    if (!lp.isEmpty()) g.lastPlayed = QDateTime::fromString(lp, Qt::ISODate);
    const QString da = obj.value(QStringLiteral("dateAdded")).toString();
    g.dateAdded = da.isEmpty() ? QDateTime::currentDateTimeUtc() : QDateTime::fromString(da, Qt::ISODate);
    return g;
}

QVariantMap EmulatorProfile::toVariantMap() const {
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("platform"), platform},
        {QStringLiteral("executablePath"), executablePath},
        {QStringLiteral("launchArguments"), launchArguments},
        {QStringLiteral("workingDirectory"), workingDirectory},
        {QStringLiteral("supportedExtensions"), supportedExtensions},
        {QStringLiteral("gameDirectories"), gameDirectories},
        {QStringLiteral("iconName"), iconName},
        {QStringLiteral("isDedicatedLichii"), isDedicatedLichii},
        {QStringLiteral("notes"), notes}
    };
}

QJsonObject EmulatorProfile::toJson() const {
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("platform"), platform},
        {QStringLiteral("executablePath"), executablePath},
        {QStringLiteral("launchArguments"), launchArguments},
        {QStringLiteral("workingDirectory"), workingDirectory},
        {QStringLiteral("supportedExtensions"), toJsonArray(supportedExtensions)},
        {QStringLiteral("gameDirectories"), toJsonArray(gameDirectories)},
        {QStringLiteral("iconName"), iconName},
        {QStringLiteral("isDedicatedLichii"), isDedicatedLichii},
        {QStringLiteral("notes"), notes}
    };
}

EmulatorProfile EmulatorProfile::fromJson(const QJsonObject &obj) {
    EmulatorProfile e;
    e.id = obj.value(QStringLiteral("id")).toString();
    e.name = obj.value(QStringLiteral("name")).toString();
    e.platform = obj.value(QStringLiteral("platform")).toString();
    e.executablePath = obj.value(QStringLiteral("executablePath")).toString();
    e.launchArguments = obj.value(QStringLiteral("launchArguments")).toString(QStringLiteral("\"{rom}\""));
    e.workingDirectory = obj.value(QStringLiteral("workingDirectory")).toString();
    e.supportedExtensions = fromJsonArray(obj.value(QStringLiteral("supportedExtensions")).toArray());
    e.gameDirectories = fromJsonArray(obj.value(QStringLiteral("gameDirectories")).toArray());
    e.iconName = obj.value(QStringLiteral("iconName")).toString(QStringLiteral("gamepad"));
    e.isDedicatedLichii = obj.value(QStringLiteral("isDedicatedLichii")).toBool(false);
    e.notes = obj.value(QStringLiteral("notes")).toString();
    return e;
}

QVariantMap CollectionEntry::toVariantMap() const {
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("description"), description},
        {QStringLiteral("iconName"), iconName},
        {QStringLiteral("accentColor"), accentColor},
        {QStringLiteral("gameIds"), gameIds},
        {QStringLiteral("gameCount"), gameIds.size()},
        {QStringLiteral("createdAt"), createdAt.isValid() ? createdAt.toString(Qt::ISODate) : QString()}
    };
}

QJsonObject CollectionEntry::toJson() const {
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("description"), description},
        {QStringLiteral("iconName"), iconName},
        {QStringLiteral("accentColor"), accentColor},
        {QStringLiteral("gameIds"), toJsonArray(gameIds)},
        {QStringLiteral("createdAt"), createdAt.isValid() ? createdAt.toString(Qt::ISODate) : QString()}
    };
}

CollectionEntry CollectionEntry::fromJson(const QJsonObject &obj) {
    CollectionEntry c;
    c.id = obj.value(QStringLiteral("id")).toString();
    c.name = obj.value(QStringLiteral("name")).toString();
    c.description = obj.value(QStringLiteral("description")).toString();
    c.iconName = obj.value(QStringLiteral("iconName")).toString(QStringLiteral("collection"));
    c.accentColor = obj.value(QStringLiteral("accentColor")).toString(QStringLiteral("#4CC2FF"));
    c.gameIds = fromJsonArray(obj.value(QStringLiteral("gameIds")).toArray());
    const QString ts = obj.value(QStringLiteral("createdAt")).toString();
    c.createdAt = ts.isEmpty() ? QDateTime::currentDateTimeUtc() : QDateTime::fromString(ts, Qt::ISODate);
    return c;
}

QVariantMap ThemeConfig::toVariantMap() const {
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("isBuiltIn"), isBuiltIn},
        {QStringLiteral("accentColor"), accentColor},
        {QStringLiteral("secondaryAccent"), secondaryAccent},
        {QStringLiteral("surfaceColor"), surfaceColor},
        {QStringLiteral("wallpaperPath"), wallpaperPath},
        {QStringLiteral("useDynamicGameBackground"), useDynamicGameBackground},
        {QStringLiteral("glassOpacity"), glassOpacity},
        {QStringLiteral("blurIntensity"), blurIntensity},
        {QStringLiteral("animationsEnabled"), animationsEnabled},
        {QStringLiteral("animationSpeed"), animationSpeed},
        {QStringLiteral("fontFamily"), fontFamily}
    };
}

QJsonObject ThemeConfig::toJson() const {
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("isBuiltIn"), isBuiltIn},
        {QStringLiteral("accentColor"), accentColor},
        {QStringLiteral("secondaryAccent"), secondaryAccent},
        {QStringLiteral("surfaceColor"), surfaceColor},
        {QStringLiteral("wallpaperPath"), wallpaperPath},
        {QStringLiteral("useDynamicGameBackground"), useDynamicGameBackground},
        {QStringLiteral("glassOpacity"), glassOpacity},
        {QStringLiteral("blurIntensity"), blurIntensity},
        {QStringLiteral("animationsEnabled"), animationsEnabled},
        {QStringLiteral("animationSpeed"), animationSpeed},
        {QStringLiteral("fontFamily"), fontFamily}
    };
}

ThemeConfig ThemeConfig::fromJson(const QJsonObject &obj) {
    ThemeConfig t;
    t.id = obj.value(QStringLiteral("id")).toString();
    t.name = obj.value(QStringLiteral("name")).toString();
    t.isBuiltIn = obj.value(QStringLiteral("isBuiltIn")).toBool(false);
    t.accentColor = obj.value(QStringLiteral("accentColor")).toString(QStringLiteral("#4CC2FF"));
    t.secondaryAccent = obj.value(QStringLiteral("secondaryAccent")).toString(QStringLiteral("#9B72FF"));
    t.surfaceColor = obj.value(QStringLiteral("surfaceColor")).toString(QStringLiteral("#0D1320"));
    t.wallpaperPath = obj.value(QStringLiteral("wallpaperPath")).toString();
    t.useDynamicGameBackground = obj.value(QStringLiteral("useDynamicGameBackground")).toBool(true);
    t.glassOpacity = obj.value(QStringLiteral("glassOpacity")).toDouble(0.24);
    t.blurIntensity = obj.value(QStringLiteral("blurIntensity")).toDouble(0.72);
    t.animationsEnabled = obj.value(QStringLiteral("animationsEnabled")).toBool(true);
    t.animationSpeed = obj.value(QStringLiteral("animationSpeed")).toDouble(1.0);
    t.fontFamily = obj.value(QStringLiteral("fontFamily")).toString(QStringLiteral("Segoe UI Variable"));
    return t;
}

QVariantMap WidgetItemConfig::toVariantMap() const {
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("enabled"), enabled},
        {QStringLiteral("order"), order},
        {QStringLiteral("size"), size}
    };
}

QJsonObject WidgetItemConfig::toJson() const {
    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("enabled"), enabled},
        {QStringLiteral("order"), order},
        {QStringLiteral("size"), size}
    };
}

WidgetItemConfig WidgetItemConfig::fromJson(const QJsonObject &obj) {
    WidgetItemConfig w;
    w.id = obj.value(QStringLiteral("id")).toString();
    w.title = obj.value(QStringLiteral("title")).toString();
    w.enabled = obj.value(QStringLiteral("enabled")).toBool(true);
    w.order = obj.value(QStringLiteral("order")).toInt(0);
    w.size = obj.value(QStringLiteral("size")).toString(QStringLiteral("medium"));
    return w;
}

QVariantMap AudioOutputDevice::toVariantMap() const {
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("isDefault"), isDefault}
    };
}

QVariantMap UpdateManifest::toVariantMap() const {
    return {
        {QStringLiteral("version"), version},
        {QStringLiteral("releaseDate"), releaseDate},
        {QStringLiteral("releaseNotes"), releaseNotes},
        {QStringLiteral("downloadUrl"), downloadUrl},
        {QStringLiteral("sha256"), sha256},
        {QStringLiteral("fileSizeBytes"), fileSizeBytes},
        {QStringLiteral("valid"), valid}
    };
}

UpdateManifest UpdateManifest::fromJson(const QJsonObject &obj) {
    UpdateManifest m;
    m.version = obj.value(QStringLiteral("version")).toString();
    m.releaseDate = obj.value(QStringLiteral("releaseDate")).toString();
    m.releaseNotes = obj.value(QStringLiteral("releaseNotes")).toString();
    m.downloadUrl = obj.value(QStringLiteral("downloadUrl")).toString();
    m.sha256 = obj.value(QStringLiteral("sha256")).toString();
    m.fileSizeBytes = static_cast<qint64>(obj.value(QStringLiteral("fileSizeBytes")).toDouble(0));
    m.valid = !m.version.isEmpty() && !m.downloadUrl.isEmpty();
    return m;
}

} // namespace Aether
