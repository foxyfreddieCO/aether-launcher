#pragma once

#include "AppPaths.h"
#include "Models.h"

#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace Aether {

class SettingsManager : public QObject {
    Q_OBJECT

    // Appearance & Liquid Glass
    Q_PROPERTY(QString activeThemeId READ activeThemeId NOTIFY settingsChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY settingsChanged)
    Q_PROPERTY(QString secondaryAccent READ secondaryAccent WRITE setSecondaryAccent NOTIFY settingsChanged)
    Q_PROPERTY(QString surfaceColor READ surfaceColor WRITE setSurfaceColor NOTIFY settingsChanged)
    Q_PROPERTY(QString wallpaperPath READ wallpaperPath WRITE setWallpaperPath NOTIFY settingsChanged)
    Q_PROPERTY(bool useDynamicGameBackground READ useDynamicGameBackground WRITE setUseDynamicGameBackground NOTIFY settingsChanged)
    Q_PROPERTY(double glassOpacity READ glassOpacity WRITE setGlassOpacity NOTIFY settingsChanged)
    Q_PROPERTY(double blurIntensity READ blurIntensity WRITE setBlurIntensity NOTIFY settingsChanged)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled NOTIFY settingsChanged)
    Q_PROPERTY(double animationSpeed READ animationSpeed WRITE setAnimationSpeed NOTIFY settingsChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY settingsChanged)
    Q_PROPERTY(QString libraryLayout READ libraryLayout WRITE setLibraryLayout NOTIFY settingsChanged)

    // Display & Modes
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY settingsChanged)
    Q_PROPERTY(bool consoleMode READ consoleMode WRITE setConsoleMode NOTIFY settingsChanged)
    Q_PROPERTY(int targetMonitorIndex READ targetMonitorIndex WRITE setTargetMonitorIndex NOTIFY settingsChanged)
    Q_PROPERTY(bool performanceOverlay READ performanceOverlay WRITE setPerformanceOverlay NOTIFY settingsChanged)
    Q_PROPERTY(bool screensaverEnabled READ screensaverEnabled WRITE setScreensaverEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int screensaverTimeoutMinutes READ screensaverTimeoutMinutes WRITE setScreensaverTimeoutMinutes NOTIFY settingsChanged)
    Q_PROPERTY(bool startupWithWindows READ startupWithWindows WRITE setStartupWithWindows NOTIFY settingsChanged)

    // Audio
    Q_PROPERTY(bool sfxEnabled READ sfxEnabled WRITE setSfxEnabled NOTIFY settingsChanged)
    Q_PROPERTY(double sfxVolume READ sfxVolume WRITE setSfxVolume NOTIFY settingsChanged)
    Q_PROPERTY(bool bgmEnabled READ bgmEnabled WRITE setBgmEnabled NOTIFY settingsChanged)
    Q_PROPERTY(double bgmVolume READ bgmVolume WRITE setBgmVolume NOTIFY settingsChanged)
    Q_PROPERTY(bool audioMuted READ audioMuted WRITE setAudioMuted NOTIFY settingsChanged)
    Q_PROPERTY(QString customSoundPackDir READ customSoundPackDir WRITE setCustomSoundPackDir NOTIFY settingsChanged)
    Q_PROPERTY(QString customBgmPath READ customBgmPath WRITE setCustomBgmPath NOTIFY settingsChanged)

    // Controller
    Q_PROPERTY(bool rumbleEnabled READ rumbleEnabled WRITE setRumbleEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap buttonMappings READ buttonMappings NOTIFY settingsChanged)

    // Metadata & Updates
    Q_PROPERTY(QString steamGridDbApiKey READ steamGridDbApiKey WRITE setSteamGridDbApiKey NOTIFY settingsChanged)
    Q_PROPERTY(QString updateManifestUrl READ updateManifestUrl WRITE setUpdateManifestUrl NOTIFY settingsChanged)
    Q_PROPERTY(bool autoCheckUpdates READ autoCheckUpdates WRITE setAutoCheckUpdates NOTIFY settingsChanged)
    Q_PROPERTY(bool autoBackupOnLaunch READ autoBackupOnLaunch WRITE setAutoBackupOnLaunch NOTIFY settingsChanged)

    // Lists
    Q_PROPERTY(QVariantList themes READ themesVariantList NOTIFY themesChanged)
    Q_PROPERTY(QVariantList widgets READ widgetsVariantList NOTIFY widgetsChanged)

public:
    explicit SettingsManager(const AppPaths &paths, QObject *parent = nullptr);

    bool load();
    bool save() const;

    // Appearance getters/setters
    QString activeThemeId() const { return m_activeThemeId; }
    QString accentColor() const { return m_accentColor; }
    void setAccentColor(const QString &v);
    QString secondaryAccent() const { return m_secondaryAccent; }
    void setSecondaryAccent(const QString &v);
    QString surfaceColor() const { return m_surfaceColor; }
    void setSurfaceColor(const QString &v);
    QString wallpaperPath() const { return m_wallpaperPath; }
    void setWallpaperPath(const QString &v);
    bool useDynamicGameBackground() const { return m_useDynamicGameBackground; }
    void setUseDynamicGameBackground(bool v);
    double glassOpacity() const { return m_glassOpacity; }
    void setGlassOpacity(double v);
    double blurIntensity() const { return m_blurIntensity; }
    void setBlurIntensity(double v);
    bool animationsEnabled() const { return m_animationsEnabled; }
    void setAnimationsEnabled(bool v);
    double animationSpeed() const { return m_animationSpeed; }
    void setAnimationSpeed(double v);
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &v);
    QString libraryLayout() const { return m_libraryLayout; }
    void setLibraryLayout(const QString &v);

    // Display getters/setters
    bool fullscreen() const { return m_fullscreen; }
    void setFullscreen(bool v);
    bool consoleMode() const { return m_consoleMode; }
    void setConsoleMode(bool v);
    int targetMonitorIndex() const { return m_targetMonitorIndex; }
    void setTargetMonitorIndex(int v);
    bool performanceOverlay() const { return m_performanceOverlay; }
    void setPerformanceOverlay(bool v);
    bool screensaverEnabled() const { return m_screensaverEnabled; }
    void setScreensaverEnabled(bool v);
    int screensaverTimeoutMinutes() const { return m_screensaverTimeoutMinutes; }
    void setScreensaverTimeoutMinutes(int v);
    bool startupWithWindows() const { return m_startupWithWindows; }
    void setStartupWithWindows(bool v);

    // Audio getters/setters
    bool sfxEnabled() const { return m_sfxEnabled; }
    void setSfxEnabled(bool v);
    double sfxVolume() const { return m_sfxVolume; }
    void setSfxVolume(double v);
    bool bgmEnabled() const { return m_bgmEnabled; }
    void setBgmEnabled(bool v);
    double bgmVolume() const { return m_bgmVolume; }
    void setBgmVolume(double v);
    bool audioMuted() const { return m_audioMuted; }
    void setAudioMuted(bool v);
    QString customSoundPackDir() const { return m_customSoundPackDir; }
    void setCustomSoundPackDir(const QString &v);
    QString customBgmPath() const { return m_customBgmPath; }
    void setCustomBgmPath(const QString &v);

    // Controller
    bool rumbleEnabled() const { return m_rumbleEnabled; }
    void setRumbleEnabled(bool v);
    QVariantMap buttonMappings() const { return m_buttonMappings; }
    Q_INVOKABLE void setButtonMapping(const QString &action, const QString &gamepadButton);
    Q_INVOKABLE void resetButtonMappings();

    // Metadata & Updates
    QString steamGridDbApiKey() const { return m_steamGridDbApiKey; }
    void setSteamGridDbApiKey(const QString &v);
    QString updateManifestUrl() const { return m_updateManifestUrl; }
    void setUpdateManifestUrl(const QString &v);
    bool autoCheckUpdates() const { return m_autoCheckUpdates; }
    void setAutoCheckUpdates(bool v);
    bool autoBackupOnLaunch() const { return m_autoBackupOnLaunch; }
    void setAutoBackupOnLaunch(bool v);

    // Themes
    QVariantList themesVariantList() const;
    Q_INVOKABLE bool applyTheme(const QString &themeId);
    Q_INVOKABLE QString saveCustomTheme(const QString &name);
    Q_INVOKABLE bool deleteCustomTheme(const QString &themeId);

    // Widgets
    QVariantList widgetsVariantList() const;
    Q_INVOKABLE void setWidgetEnabled(const QString &widgetId, bool enabled);
    Q_INVOKABLE void setWidgetSize(const QString &widgetId, const QString &size);
    Q_INVOKABLE void moveWidget(const QString &widgetId, int delta);
    Q_INVOKABLE void resetWidgets();

    // Serialization for Import/Export
    QJsonObject exportToJson() const;
    bool importFromJson(const QJsonObject &obj);

signals:
    void settingsChanged();
    void themesChanged();
    void widgetsChanged();
    void startupWithWindowsChanged(bool enabled);

private:
    void initBuiltInThemes();
    void initDefaultWidgets();
    void initDefaultButtonMappings();

    AppPaths m_paths;

    QString m_activeThemeId = QStringLiteral("ps5-sapphire");
    QString m_accentColor = QStringLiteral("#4CC2FF");
    QString m_secondaryAccent = QStringLiteral("#8B6DFF");
    QString m_surfaceColor = QStringLiteral("#0B1120");
    QString m_wallpaperPath;
    bool m_useDynamicGameBackground = true;
    double m_glassOpacity = 0.24;
    double m_blurIntensity = 0.72;
    bool m_animationsEnabled = true;
    double m_animationSpeed = 1.0;
    QString m_fontFamily = QStringLiteral("Segoe UI Variable");
    QString m_libraryLayout = QStringLiteral("ribbon"); // ribbon, grid, list

    bool m_fullscreen = true;
    bool m_consoleMode = true;
    int m_targetMonitorIndex = 0;
    bool m_performanceOverlay = false;
    bool m_screensaverEnabled = true;
    int m_screensaverTimeoutMinutes = 5;
    bool m_startupWithWindows = false;

    bool m_sfxEnabled = true;
    double m_sfxVolume = 0.75;
    bool m_bgmEnabled = false;
    double m_bgmVolume = 0.35;
    bool m_audioMuted = false;
    QString m_customSoundPackDir;
    QString m_customBgmPath;

    bool m_rumbleEnabled = true;
    QVariantMap m_buttonMappings;

    QString m_steamGridDbApiKey;
    QString m_updateManifestUrl;
    bool m_autoCheckUpdates = true;
    bool m_autoBackupOnLaunch = true;

    QList<ThemeConfig> m_themes;
    QList<WidgetItemConfig> m_widgets;
};

} // namespace Aether
