#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Aether {

struct GameEntry {
    QString id;
    QString title;
    QString platform;           // e.g. "Nintendo Switch 2", "Windows", "PlayStation 2", "GameCube"
    QString category;           // e.g. "Action", "RPG", "Indie", "Uncategorized"
    QString description;
    QString executablePath;     // Direct .exe path OR ROM/game file path
    QString workingDirectory;
    QString emulatorId;         // Empty for native Windows .exe / direct apps
    QString customArguments;    // Per-game override arguments
    QString coverPath;          // Local cached or user-selected cover image path
    QString backgroundPath;     // Local cached or user-selected hero background path
    QString developer;
    QString releaseYear;
    bool isFavorite = false;
    qint64 playtimeSeconds = 0;
    int playCount = 0;
    QDateTime lastPlayed;
    QDateTime dateAdded;

    QVariantMap toVariantMap() const;
    QJsonObject toJson() const;
    static GameEntry fromJson(const QJsonObject &obj);
};

struct EmulatorProfile {
    QString id;
    QString name;               // e.g. "Lichii"
    QString platform;           // e.g. "Nintendo Switch 2"
    QString executablePath;     // User-configured path to emulator binary
    QString launchArguments;    // Template: e.g. "\"{rom}\"" (user-configurable!)
    QString workingDirectory;
    QStringList supportedExtensions; // e.g. {".nsp", ".xci"}
    QStringList gameDirectories;     // User-configured directories to scan
    QString iconName;
    bool isDedicatedLichii = false;  // Marks the first-class Lichii Switch 2 integration
    QString notes;

    QVariantMap toVariantMap() const;
    QJsonObject toJson() const;
    static EmulatorProfile fromJson(const QJsonObject &obj);
};

struct CollectionEntry {
    QString id;
    QString name;
    QString description;
    QString iconName;
    QString accentColor;
    QStringList gameIds;
    QDateTime createdAt;

    QVariantMap toVariantMap() const;
    QJsonObject toJson() const;
    static CollectionEntry fromJson(const QJsonObject &obj);
};

struct ThemeConfig {
    QString id;
    QString name;
    bool isBuiltIn = false;
    QString accentColor = QStringLiteral("#4CC2FF");
    QString secondaryAccent = QStringLiteral("#9B72FF");
    QString surfaceColor = QStringLiteral("#0D1320");
    QString wallpaperPath;
    bool useDynamicGameBackground = true;
    double glassOpacity = 0.24;     // 0.08 - 0.85
    double blurIntensity = 0.72;    // 0.0 - 1.0
    bool animationsEnabled = true;
    double animationSpeed = 1.0;    // 0.5 - 2.0
    QString fontFamily = QStringLiteral("Segoe UI Variable");

    QVariantMap toVariantMap() const;
    QJsonObject toJson() const;
    static ThemeConfig fromJson(const QJsonObject &obj);
};

struct WidgetItemConfig {
    QString id;          // clock, telemetry, audio, controller, network, bluetooth, recent, stats
    QString title;
    bool enabled = true;
    int order = 0;
    QString size = QStringLiteral("medium"); // compact, medium, large

    QVariantMap toVariantMap() const;
    QJsonObject toJson() const;
    static WidgetItemConfig fromJson(const QJsonObject &obj);
};

struct AudioOutputDevice {
    QString id;
    QString name;
    bool isDefault = false;

    QVariantMap toVariantMap() const;
};

struct SystemTelemetry {
    double cpuUsagePercent = 0.0;
    double gpuUsagePercent = -1.0;   // -1 indicates unavailable on current hardware/driver
    double ramUsagePercent = 0.0;
    double ramUsedGb = 0.0;
    double ramTotalGb = 0.0;
    double temperatureCelsius = -1.0; // -1 indicates sensor unavailable
    bool gpuAvailable = false;
    bool temperatureAvailable = false;
};

struct UpdateManifest {
    QString version;
    QString releaseDate;
    QString releaseNotes;
    QString downloadUrl;
    QString sha256;
    qint64 fileSizeBytes = 0;
    bool valid = false;

    QVariantMap toVariantMap() const;
    static UpdateManifest fromJson(const QJsonObject &obj);
};

} // namespace Aether
