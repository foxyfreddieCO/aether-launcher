#include "SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <algorithm>

namespace Aether {

SettingsManager::SettingsManager(const AppPaths &paths, QObject *parent)
    : QObject(parent)
    , m_paths(paths) {
    initBuiltInThemes();
    initDefaultWidgets();
    initDefaultButtonMappings();
}

void SettingsManager::initBuiltInThemes() {
    m_themes.clear();

    ThemeConfig sapphire;
    sapphire.id = QStringLiteral("ps5-sapphire");
    sapphire.name = QStringLiteral("PS5 Sapphire");
    sapphire.isBuiltIn = true;
    sapphire.accentColor = QStringLiteral("#4CC2FF");
    sapphire.secondaryAccent = QStringLiteral("#7A5CFF");
    sapphire.surfaceColor = QStringLiteral("#090F1E");
    sapphire.glassOpacity = 0.24;
    sapphire.blurIntensity = 0.76;
    sapphire.fontFamily = QStringLiteral("Segoe UI Variable");

    ThemeConfig obsidian;
    obsidian.id = QStringLiteral("obsidian-liquid");
    obsidian.name = QStringLiteral("Obsidian Liquid");
    obsidian.isBuiltIn = true;
    obsidian.accentColor = QStringLiteral("#E2E8F0");
    obsidian.secondaryAccent = QStringLiteral("#64748B");
    obsidian.surfaceColor = QStringLiteral("#07080C");
    obsidian.glassOpacity = 0.28;
    obsidian.blurIntensity = 0.85;
    obsidian.fontFamily = QStringLiteral("Segoe UI Variable");

    ThemeConfig aurora;
    aurora.id = QStringLiteral("aurora-frost");
    aurora.name = QStringLiteral("Aurora Frost");
    aurora.isBuiltIn = true;
    aurora.accentColor = QStringLiteral("#2EE6B8");
    aurora.secondaryAccent = QStringLiteral("#38BDF8");
    aurora.surfaceColor = QStringLiteral("#071820");
    aurora.glassOpacity = 0.22;
    aurora.blurIntensity = 0.80;
    aurora.fontFamily = QStringLiteral("Segoe UI Variable");

    ThemeConfig ember;
    ember.id = QStringLiteral("crimson-ember");
    ember.name = QStringLiteral("Crimson Ember");
    ember.isBuiltIn = true;
    ember.accentColor = QStringLiteral("#FF5C7A");
    ember.secondaryAccent = QStringLiteral("#FF9E44");
    ember.surfaceColor = QStringLiteral("#170A12");
    ember.glassOpacity = 0.26;
    ember.blurIntensity = 0.74;
    ember.fontFamily = QStringLiteral("Segoe UI Variable");

    ThemeConfig switchPrism;
    switchPrism.id = QStringLiteral("prism-nebula");
    switchPrism.name = QStringLiteral("Prism Nebula");
    switchPrism.isBuiltIn = true;
    switchPrism.accentColor = QStringLiteral("#38D6FF");
    switchPrism.secondaryAccent = QStringLiteral("#FF4D6D");
    switchPrism.surfaceColor = QStringLiteral("#0B0D18");
    switchPrism.glassOpacity = 0.22;
    switchPrism.blurIntensity = 0.78;
    switchPrism.fontFamily = QStringLiteral("Segoe UI Variable");

    m_themes << sapphire << obsidian << aurora << ember << switchPrism;
}

void SettingsManager::initDefaultWidgets() {
    m_widgets = {
        {QStringLiteral("clock"),      QStringLiteral("Clock & Date"),       true,  0, QStringLiteral("medium")},
        {QStringLiteral("telemetry"),  QStringLiteral("CPU / GPU / RAM"),    true,  1, QStringLiteral("medium")},
        {QStringLiteral("audio"),      QStringLiteral("Volume & Audio"),     true,  2, QStringLiteral("compact")},
        {QStringLiteral("controller"), QStringLiteral("Controller Status"),  true,  3, QStringLiteral("medium")},
        {QStringLiteral("wifi"),       QStringLiteral("Wi-Fi Shortcut"),     true,  4, QStringLiteral("compact")},
        {QStringLiteral("bluetooth"),  QStringLiteral("Bluetooth Shortcut"), true,  5, QStringLiteral("compact")},
        {QStringLiteral("recent"),     QStringLiteral("Recently Played"),    true,  6, QStringLiteral("medium")},
        {QStringLiteral("stats"),      QStringLiteral("Gaming Statistics"),  true,  7, QStringLiteral("medium")}
    };
}

void SettingsManager::initDefaultButtonMappings() {
    // Matches Section 5 default controls:
    // A/Cross: Confirm, B/Circle: Back, X/Square: Info, Y/Triangle: Options,
    // LB/RB: Switch categories, LT/RT: Navigate collections, Start: Quick menu
    m_buttonMappings = {
        {QStringLiteral("confirm"),        QStringLiteral("south")},
        {QStringLiteral("back"),           QStringLiteral("east")},
        {QStringLiteral("info"),           QStringLiteral("west")},
        {QStringLiteral("options"),        QStringLiteral("north")},
        {QStringLiteral("prevCategory"),   QStringLiteral("left_shoulder")},
        {QStringLiteral("nextCategory"),   QStringLiteral("right_shoulder")},
        {QStringLiteral("prevCollection"), QStringLiteral("left_trigger")},
        {QStringLiteral("nextCollection"), QStringLiteral("right_trigger")},
        {QStringLiteral("quickMenu"),      QStringLiteral("start")}
    };
}

bool SettingsManager::load() {
    QFile file(m_paths.settingsFile());
    if (!file.exists()) {
        return save();
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }
    return importFromJson(doc.object());
}

bool SettingsManager::save() const {
    m_paths.ensureDirectoriesExist();
    const QString path = m_paths.settingsFile();
    const QString tmpPath = path + QStringLiteral(".tmp");

    QFile file(tmpPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QJsonDocument doc(exportToJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.flush();
    file.close();

    QFile::remove(path);
    return QFile::rename(tmpPath, path);
}

QJsonObject SettingsManager::exportToJson() const {
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("activeThemeId"), m_activeThemeId);
    root.insert(QStringLiteral("accentColor"), m_accentColor);
    root.insert(QStringLiteral("secondaryAccent"), m_secondaryAccent);
    root.insert(QStringLiteral("surfaceColor"), m_surfaceColor);
    root.insert(QStringLiteral("wallpaperPath"), m_wallpaperPath);
    root.insert(QStringLiteral("useDynamicGameBackground"), m_useDynamicGameBackground);
    root.insert(QStringLiteral("glassOpacity"), m_glassOpacity);
    root.insert(QStringLiteral("blurIntensity"), m_blurIntensity);
    root.insert(QStringLiteral("animationsEnabled"), m_animationsEnabled);
    root.insert(QStringLiteral("animationSpeed"), m_animationSpeed);
    root.insert(QStringLiteral("fontFamily"), m_fontFamily);
    root.insert(QStringLiteral("libraryLayout"), m_libraryLayout);

    root.insert(QStringLiteral("fullscreen"), m_fullscreen);
    root.insert(QStringLiteral("consoleMode"), m_consoleMode);
    root.insert(QStringLiteral("targetMonitorIndex"), m_targetMonitorIndex);
    root.insert(QStringLiteral("performanceOverlay"), m_performanceOverlay);
    root.insert(QStringLiteral("screensaverEnabled"), m_screensaverEnabled);
    root.insert(QStringLiteral("screensaverTimeoutMinutes"), m_screensaverTimeoutMinutes);
    root.insert(QStringLiteral("startupWithWindows"), m_startupWithWindows);

    root.insert(QStringLiteral("sfxEnabled"), m_sfxEnabled);
    root.insert(QStringLiteral("sfxVolume"), m_sfxVolume);
    root.insert(QStringLiteral("bgmEnabled"), m_bgmEnabled);
    root.insert(QStringLiteral("bgmVolume"), m_bgmVolume);
    root.insert(QStringLiteral("audioMuted"), m_audioMuted);
    root.insert(QStringLiteral("customSoundPackDir"), m_customSoundPackDir);
    root.insert(QStringLiteral("customBgmPath"), m_customBgmPath);

    root.insert(QStringLiteral("rumbleEnabled"), m_rumbleEnabled);
    root.insert(QStringLiteral("buttonMappings"), QJsonObject::fromVariantMap(m_buttonMappings));

    root.insert(QStringLiteral("steamGridDbApiKey"), m_steamGridDbApiKey);
    root.insert(QStringLiteral("updateManifestUrl"), m_updateManifestUrl);
    root.insert(QStringLiteral("autoCheckUpdates"), m_autoCheckUpdates);
    root.insert(QStringLiteral("autoBackupOnLaunch"), m_autoBackupOnLaunch);

    QJsonArray customThemes;
    for (const auto &t : m_themes) {
        if (!t.isBuiltIn) {
            customThemes.append(t.toJson());
        }
    }
    root.insert(QStringLiteral("customThemes"), customThemes);

    QJsonArray widgetsArr;
    for (const auto &w : m_widgets) {
        widgetsArr.append(w.toJson());
    }
    root.insert(QStringLiteral("widgets"), widgetsArr);

    return root;
}

bool SettingsManager::importFromJson(const QJsonObject &obj) {
    m_activeThemeId = obj.value(QStringLiteral("activeThemeId")).toString(m_activeThemeId);
    m_accentColor = obj.value(QStringLiteral("accentColor")).toString(m_accentColor);
    m_secondaryAccent = obj.value(QStringLiteral("secondaryAccent")).toString(m_secondaryAccent);
    m_surfaceColor = obj.value(QStringLiteral("surfaceColor")).toString(m_surfaceColor);
    m_wallpaperPath = obj.value(QStringLiteral("wallpaperPath")).toString(m_wallpaperPath);
    m_useDynamicGameBackground = obj.value(QStringLiteral("useDynamicGameBackground")).toBool(m_useDynamicGameBackground);
    m_glassOpacity = qBound(0.06, obj.value(QStringLiteral("glassOpacity")).toDouble(m_glassOpacity), 0.85);
    m_blurIntensity = qBound(0.0, obj.value(QStringLiteral("blurIntensity")).toDouble(m_blurIntensity), 1.0);
    m_animationsEnabled = obj.value(QStringLiteral("animationsEnabled")).toBool(m_animationsEnabled);
    m_animationSpeed = qBound(0.4, obj.value(QStringLiteral("animationSpeed")).toDouble(m_animationSpeed), 2.5);
    m_fontFamily = obj.value(QStringLiteral("fontFamily")).toString(m_fontFamily);
    m_libraryLayout = obj.value(QStringLiteral("libraryLayout")).toString(m_libraryLayout);

    m_fullscreen = obj.value(QStringLiteral("fullscreen")).toBool(m_fullscreen);
    m_consoleMode = obj.value(QStringLiteral("consoleMode")).toBool(m_consoleMode);
    m_targetMonitorIndex = qMax(0, obj.value(QStringLiteral("targetMonitorIndex")).toInt(m_targetMonitorIndex));
    m_performanceOverlay = obj.value(QStringLiteral("performanceOverlay")).toBool(m_performanceOverlay);
    m_screensaverEnabled = obj.value(QStringLiteral("screensaverEnabled")).toBool(m_screensaverEnabled);
    m_screensaverTimeoutMinutes = qBound(1, obj.value(QStringLiteral("screensaverTimeoutMinutes")).toInt(m_screensaverTimeoutMinutes), 60);
    m_startupWithWindows = obj.value(QStringLiteral("startupWithWindows")).toBool(m_startupWithWindows);

    m_sfxEnabled = obj.value(QStringLiteral("sfxEnabled")).toBool(m_sfxEnabled);
    m_sfxVolume = qBound(0.0, obj.value(QStringLiteral("sfxVolume")).toDouble(m_sfxVolume), 1.0);
    m_bgmEnabled = obj.value(QStringLiteral("bgmEnabled")).toBool(m_bgmEnabled);
    m_bgmVolume = qBound(0.0, obj.value(QStringLiteral("bgmVolume")).toDouble(m_bgmVolume), 1.0);
    m_audioMuted = obj.value(QStringLiteral("audioMuted")).toBool(m_audioMuted);
    m_customSoundPackDir = obj.value(QStringLiteral("customSoundPackDir")).toString(m_customSoundPackDir);
    m_customBgmPath = obj.value(QStringLiteral("customBgmPath")).toString(m_customBgmPath);

    m_rumbleEnabled = obj.value(QStringLiteral("rumbleEnabled")).toBool(m_rumbleEnabled);
    if (obj.value(QStringLiteral("buttonMappings")).isObject()) {
        const QVariantMap loaded = obj.value(QStringLiteral("buttonMappings")).toObject().toVariantMap();
        for (auto it = loaded.constBegin(); it != loaded.constEnd(); ++it) {
            m_buttonMappings.insert(it.key(), it.value());
        }
    }

    m_steamGridDbApiKey = obj.value(QStringLiteral("steamGridDbApiKey")).toString(m_steamGridDbApiKey);
    m_updateManifestUrl = obj.value(QStringLiteral("updateManifestUrl")).toString(m_updateManifestUrl);
    m_autoCheckUpdates = obj.value(QStringLiteral("autoCheckUpdates")).toBool(m_autoCheckUpdates);
    m_autoBackupOnLaunch = obj.value(QStringLiteral("autoBackupOnLaunch")).toBool(m_autoBackupOnLaunch);

    initBuiltInThemes();
    const QJsonArray customThemes = obj.value(QStringLiteral("customThemes")).toArray();
    for (const auto &v : customThemes) {
        if (v.isObject()) {
            ThemeConfig tc = ThemeConfig::fromJson(v.toObject());
            tc.isBuiltIn = false;
            if (!tc.id.isEmpty() && !tc.name.isEmpty()) {
                m_themes.append(tc);
            }
        }
    }

    if (obj.value(QStringLiteral("widgets")).isArray()) {
        QList<WidgetItemConfig> loadedWidgets;
        const QJsonArray wArr = obj.value(QStringLiteral("widgets")).toArray();
        for (const auto &v : wArr) {
            if (v.isObject()) {
                WidgetItemConfig w = WidgetItemConfig::fromJson(v.toObject());
                if (!w.id.isEmpty()) loadedWidgets.append(w);
            }
        }
        if (!loadedWidgets.isEmpty()) {
            std::sort(loadedWidgets.begin(), loadedWidgets.end(),
                      [](const WidgetItemConfig &a, const WidgetItemConfig &b) { return a.order < b.order; });
            m_widgets = loadedWidgets;
        }
    }

    emit settingsChanged();
    emit themesChanged();
    emit widgetsChanged();
    return true;
}

#define SETTER_IMPL(Type, Field, Name) \
void SettingsManager::Name(Type v) { \
    if (Field == v) return; \
    Field = v; \
    save(); \
    emit settingsChanged(); \
}

SETTER_IMPL(const QString &, m_accentColor, setAccentColor)
SETTER_IMPL(const QString &, m_secondaryAccent, setSecondaryAccent)
SETTER_IMPL(const QString &, m_surfaceColor, setSurfaceColor)
SETTER_IMPL(const QString &, m_wallpaperPath, setWallpaperPath)
SETTER_IMPL(bool, m_useDynamicGameBackground, setUseDynamicGameBackground)
SETTER_IMPL(bool, m_animationsEnabled, setAnimationsEnabled)
SETTER_IMPL(const QString &, m_fontFamily, setFontFamily)
SETTER_IMPL(const QString &, m_libraryLayout, setLibraryLayout)
SETTER_IMPL(bool, m_fullscreen, setFullscreen)
SETTER_IMPL(bool, m_consoleMode, setConsoleMode)
SETTER_IMPL(int, m_targetMonitorIndex, setTargetMonitorIndex)
SETTER_IMPL(bool, m_performanceOverlay, setPerformanceOverlay)
SETTER_IMPL(bool, m_screensaverEnabled, setScreensaverEnabled)
SETTER_IMPL(bool, m_sfxEnabled, setSfxEnabled)
SETTER_IMPL(bool, m_bgmEnabled, setBgmEnabled)
SETTER_IMPL(bool, m_audioMuted, setAudioMuted)
SETTER_IMPL(const QString &, m_customSoundPackDir, setCustomSoundPackDir)
SETTER_IMPL(const QString &, m_customBgmPath, setCustomBgmPath)
SETTER_IMPL(bool, m_rumbleEnabled, setRumbleEnabled)
SETTER_IMPL(const QString &, m_steamGridDbApiKey, setSteamGridDbApiKey)
SETTER_IMPL(const QString &, m_updateManifestUrl, setUpdateManifestUrl)
SETTER_IMPL(bool, m_autoCheckUpdates, setAutoCheckUpdates)
SETTER_IMPL(bool, m_autoBackupOnLaunch, setAutoBackupOnLaunch)

void SettingsManager::setGlassOpacity(double v) {
    v = qBound(0.06, v, 0.85);
    if (qFuzzyCompare(m_glassOpacity, v)) return;
    m_glassOpacity = v;
    save();
    emit settingsChanged();
}

void SettingsManager::setBlurIntensity(double v) {
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(m_blurIntensity, v)) return;
    m_blurIntensity = v;
    save();
    emit settingsChanged();
}

void SettingsManager::setAnimationSpeed(double v) {
    v = qBound(0.4, v, 2.5);
    if (qFuzzyCompare(m_animationSpeed, v)) return;
    m_animationSpeed = v;
    save();
    emit settingsChanged();
}

void SettingsManager::setScreensaverTimeoutMinutes(int v) {
    v = qBound(1, v, 60);
    if (m_screensaverTimeoutMinutes == v) return;
    m_screensaverTimeoutMinutes = v;
    save();
    emit settingsChanged();
}

void SettingsManager::setStartupWithWindows(bool v) {
    if (m_startupWithWindows == v) return;
    m_startupWithWindows = v;
    save();
    emit settingsChanged();
    emit startupWithWindowsChanged(v);
}

void SettingsManager::setSfxVolume(double v) {
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(m_sfxVolume, v)) return;
    m_sfxVolume = v;
    save();
    emit settingsChanged();
}

void SettingsManager::setBgmVolume(double v) {
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(m_bgmVolume, v)) return;
    m_bgmVolume = v;
    save();
    emit settingsChanged();
}

void SettingsManager::setButtonMapping(const QString &action, const QString &gamepadButton) {
    if (action.isEmpty() || gamepadButton.isEmpty()) return;
    m_buttonMappings.insert(action, gamepadButton);
    save();
    emit settingsChanged();
}

void SettingsManager::resetButtonMappings() {
    initDefaultButtonMappings();
    save();
    emit settingsChanged();
}

QVariantList SettingsManager::themesVariantList() const {
    QVariantList out;
    for (const auto &t : m_themes) {
        out.append(t.toVariantMap());
    }
    return out;
}

bool SettingsManager::applyTheme(const QString &themeId) {
    for (const auto &t : m_themes) {
        if (t.id == themeId) {
            m_activeThemeId = t.id;
            m_accentColor = t.accentColor;
            m_secondaryAccent = t.secondaryAccent;
            m_surfaceColor = t.surfaceColor;
            m_wallpaperPath = t.wallpaperPath;
            m_useDynamicGameBackground = t.useDynamicGameBackground;
            m_glassOpacity = t.glassOpacity;
            m_blurIntensity = t.blurIntensity;
            m_animationsEnabled = t.animationsEnabled;
            m_animationSpeed = t.animationSpeed;
            m_fontFamily = t.fontFamily;
            save();
            emit settingsChanged();
            return true;
        }
    }
    return false;
}

QString SettingsManager::saveCustomTheme(const QString &name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) return {};

    ThemeConfig tc;
    tc.id = QStringLiteral("custom-") + QUuid::createUuid().toString(QUuid::Id128).left(8);
    tc.name = trimmed;
    tc.isBuiltIn = false;
    tc.accentColor = m_accentColor;
    tc.secondaryAccent = m_secondaryAccent;
    tc.surfaceColor = m_surfaceColor;
    tc.wallpaperPath = m_wallpaperPath;
    tc.useDynamicGameBackground = m_useDynamicGameBackground;
    tc.glassOpacity = m_glassOpacity;
    tc.blurIntensity = m_blurIntensity;
    tc.animationsEnabled = m_animationsEnabled;
    tc.animationSpeed = m_animationSpeed;
    tc.fontFamily = m_fontFamily;

    m_themes.append(tc);
    m_activeThemeId = tc.id;
    save();
    emit themesChanged();
    emit settingsChanged();
    return tc.id;
}

bool SettingsManager::deleteCustomTheme(const QString &themeId) {
    for (int i = 0; i < m_themes.size(); ++i) {
        if (m_themes[i].id == themeId && !m_themes[i].isBuiltIn) {
            m_themes.removeAt(i);
            if (m_activeThemeId == themeId) {
                applyTheme(QStringLiteral("ps5-sapphire"));
            } else {
                save();
            }
            emit themesChanged();
            return true;
        }
    }
    return false;
}

QVariantList SettingsManager::widgetsVariantList() const {
    QVariantList out;
    for (const auto &w : m_widgets) {
        out.append(w.toVariantMap());
    }
    return out;
}

void SettingsManager::setWidgetEnabled(const QString &widgetId, bool enabled) {
    for (auto &w : m_widgets) {
        if (w.id == widgetId) {
            w.enabled = enabled;
            save();
            emit widgetsChanged();
            return;
        }
    }
}

void SettingsManager::setWidgetSize(const QString &widgetId, const QString &size) {
    if (size != QLatin1String("compact") && size != QLatin1String("medium") && size != QLatin1String("large")) {
        return;
    }
    for (auto &w : m_widgets) {
        if (w.id == widgetId) {
            w.size = size;
            save();
            emit widgetsChanged();
            return;
        }
    }
}

void SettingsManager::moveWidget(const QString &widgetId, int delta) {
    for (int i = 0; i < m_widgets.size(); ++i) {
        if (m_widgets[i].id == widgetId) {
            const int target = qBound(0, i + delta, static_cast<int>(m_widgets.size()) - 1);
            if (target == i) return;
            m_widgets.swapItemsAt(i, target);
            for (int k = 0; k < m_widgets.size(); ++k) {
                m_widgets[k].order = k;
            }
            save();
            emit widgetsChanged();
            return;
        }
    }
}

void SettingsManager::resetWidgets() {
    initDefaultWidgets();
    save();
    emit widgetsChanged();
}

} // namespace Aether
