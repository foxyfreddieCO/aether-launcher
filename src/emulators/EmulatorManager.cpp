#include "EmulatorManager.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>

namespace Aether {

EmulatorManager::EmulatorManager(Database &database, QObject *parent)
    : QObject(parent)
    , m_db(database) {}

void EmulatorManager::initializeDefaults() {
    reload();
    // Ensure dedicated configurable Lichii profile always exists (never hardcodes exe path or undocumented CLI flags)
    if (!findById(QString::fromLatin1(kLichiiId)).has_value()) {
        EmulatorProfile lichii;
        lichii.id = QString::fromLatin1(kLichiiId);
        lichii.name = QStringLiteral("Lichii");
        lichii.platform = QStringLiteral("Nintendo Switch 2");
        lichii.executablePath = QString(); // User selects their local Lichii executable
        lichii.launchArguments = QStringLiteral("\"{rom}\""); // User-configurable template
        lichii.supportedExtensions = {QStringLiteral(".nsp"), QStringLiteral(".xci"), QStringLiteral(".nro")};
        lichii.iconName = QStringLiteral("switch2");
        lichii.isDedicatedLichii = true;
        lichii.notes = QStringLiteral(
            "Dedicated configurable integration for Lichii (Nintendo Switch 2). "
            "Browse to your Lichii executable, add your game folders, and customize launch arguments "
            "using {rom}, {rom_dir}, or {rom_name} as needed. No firmware, keys, or ROMs are bundled.");
        m_db.upsertEmulator(lichii);
        reload();
    }
}

void EmulatorManager::reload() {
    m_emulators = m_db.getAllEmulators();
    emit emulatorsChanged();
}

QVariantList EmulatorManager::emulatorsVariantList() const {
    QVariantList out;
    for (const auto &e : m_emulators) {
        QVariantMap map = e.toVariantMap();
        map.insert(QStringLiteral("executableExists"), !e.executablePath.isEmpty() && QFileInfo::exists(e.executablePath));
        out.append(map);
    }
    return out;
}

QVariantMap EmulatorManager::lichiiProfileVariant() const {
    auto opt = findById(QString::fromLatin1(kLichiiId));
    if (!opt.has_value()) return {};
    QVariantMap map = opt->toVariantMap();
    map.insert(QStringLiteral("executableExists"), !opt->executablePath.isEmpty() && QFileInfo::exists(opt->executablePath));
    return map;
}

std::optional<EmulatorProfile> EmulatorManager::findById(const QString &id) const {
    for (const auto &e : m_emulators) {
        if (e.id == id) return e;
    }
    return std::nullopt;
}

std::optional<EmulatorProfile> EmulatorManager::matchExtension(const QString &filePath) const {
    const QFileInfo fi(filePath);
    const QString suffix = QStringLiteral(".") + fi.suffix().toLower();
    if (suffix == QLatin1String(".")) return std::nullopt;

    for (const auto &e : m_emulators) {
        for (const QString &ext : e.supportedExtensions) {
            if (ext.toLower() == suffix) {
                return e;
            }
        }
    }
    return std::nullopt;
}

QStringList EmulatorManager::normalizeExtensions(const QStringList &raw) {
    QStringList out;
    for (QString ext : raw) {
        ext = ext.trimmed().toLower();
        if (ext.isEmpty()) continue;
        if (!ext.startsWith(QLatin1Char('.'))) {
            ext.prepend(QLatin1Char('.'));
        }
        if (!out.contains(ext)) {
            out.append(ext);
        }
    }
    return out;
}

QString EmulatorManager::addEmulator(const QVariantMap &data) {
    EmulatorProfile e;
    e.id = data.value(QStringLiteral("id")).toString();
    if (e.id.isEmpty()) {
        e.id = QStringLiteral("emu-") + QUuid::createUuid().toString(QUuid::Id128).left(10);
    }
    e.name = data.value(QStringLiteral("name")).toString().trimmed();
    if (e.name.isEmpty()) return {};

    e.platform = data.value(QStringLiteral("platform"), QStringLiteral("Custom")).toString().trimmed();
    e.executablePath = QDir::cleanPath(data.value(QStringLiteral("executablePath")).toString().trimmed());
    e.launchArguments = data.value(QStringLiteral("launchArguments"), QStringLiteral("\"{rom}\"")).toString();
    e.workingDirectory = data.value(QStringLiteral("workingDirectory")).toString().trimmed();
    e.supportedExtensions = normalizeExtensions(data.value(QStringLiteral("supportedExtensions")).toStringList());
    e.gameDirectories = data.value(QStringLiteral("gameDirectories")).toStringList();
    e.iconName = data.value(QStringLiteral("iconName"), QStringLiteral("gamepad")).toString();
    e.isDedicatedLichii = data.value(QStringLiteral("isDedicatedLichii"), false).toBool();
    e.notes = data.value(QStringLiteral("notes")).toString();

    if (!m_db.upsertEmulator(e)) return {};
    reload();
    return e.id;
}

bool EmulatorManager::updateEmulator(const QString &id, const QVariantMap &data) {
    auto existing = findById(id);
    if (!existing.has_value()) return false;

    EmulatorProfile e = *existing;
    if (data.contains(QStringLiteral("name"))) e.name = data.value(QStringLiteral("name")).toString().trimmed();
    if (data.contains(QStringLiteral("platform"))) e.platform = data.value(QStringLiteral("platform")).toString().trimmed();
    if (data.contains(QStringLiteral("executablePath"))) {
        const QString rawPath = data.value(QStringLiteral("executablePath")).toString().trimmed();
        e.executablePath = rawPath.isEmpty() ? QString() : QDir::cleanPath(rawPath);
    }
    if (data.contains(QStringLiteral("launchArguments"))) e.launchArguments = data.value(QStringLiteral("launchArguments")).toString();
    if (data.contains(QStringLiteral("workingDirectory"))) e.workingDirectory = data.value(QStringLiteral("workingDirectory")).toString().trimmed();
    if (data.contains(QStringLiteral("supportedExtensions"))) {
        if (data.value(QStringLiteral("supportedExtensions")).typeId() == QMetaType::QString) {
            const QStringList split = data.value(QStringLiteral("supportedExtensions")).toString().split(QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts);
            e.supportedExtensions = normalizeExtensions(split);
        } else {
            e.supportedExtensions = normalizeExtensions(data.value(QStringLiteral("supportedExtensions")).toStringList());
        }
    }
    if (data.contains(QStringLiteral("gameDirectories"))) e.gameDirectories = data.value(QStringLiteral("gameDirectories")).toStringList();
    if (data.contains(QStringLiteral("notes"))) e.notes = data.value(QStringLiteral("notes")).toString();

    if (!m_db.upsertEmulator(e)) return false;
    reload();
    return true;
}

bool EmulatorManager::removeEmulator(const QString &id) {
    // Protect dedicated Lichii profile from accidental deletion (can still be cleared/reconfigured)
    if (id == QLatin1String(kLichiiId)) {
        return false;
    }
    if (!m_db.removeEmulator(id)) return false;
    reload();
    return true;
}

bool EmulatorManager::addGameDirectory(const QString &emulatorId, const QString &directory) {
    auto existing = findById(emulatorId);
    if (!existing.has_value()) return false;

    const QString cleaned = QDir::cleanPath(directory.trimmed());
    if (cleaned.isEmpty()) return false;

    EmulatorProfile e = *existing;
    if (!e.gameDirectories.contains(cleaned)) {
        e.gameDirectories.append(cleaned);
        if (!m_db.upsertEmulator(e)) return false;
        reload();
    }
    return true;
}

bool EmulatorManager::removeGameDirectory(const QString &emulatorId, const QString &directory) {
    auto existing = findById(emulatorId);
    if (!existing.has_value()) return false;

    EmulatorProfile e = *existing;
    if (e.gameDirectories.removeAll(directory) > 0) {
        if (!m_db.upsertEmulator(e)) return false;
        reload();
        return true;
    }
    return false;
}

QVariantList EmulatorManager::presetTemplates() const {
    return {
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Dolphin")},
            {QStringLiteral("platform"), QStringLiteral("GameCube / Wii")},
            {QStringLiteral("launchArguments"), QStringLiteral("-b -e \"{rom}\"")},
            {QStringLiteral("supportedExtensions"), QStringList{QStringLiteral(".iso"), QStringLiteral(".rvz"), QStringLiteral(".gcz"), QStringLiteral(".wbfs")}}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("PCSX2")},
            {QStringLiteral("platform"), QStringLiteral("PlayStation 2")},
            {QStringLiteral("launchArguments"), QStringLiteral("-fullscreen -batch \"{rom}\"")},
            {QStringLiteral("supportedExtensions"), QStringList{QStringLiteral(".iso"), QStringLiteral(".chd"), QStringLiteral(".cso")}}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("RPCS3")},
            {QStringLiteral("platform"), QStringLiteral("PlayStation 3")},
            {QStringLiteral("launchArguments"), QStringLiteral("--no-gui \"{rom}\"")},
            {QStringLiteral("supportedExtensions"), QStringList{QStringLiteral(".bin"), QStringLiteral(".pkg")}}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Cemu")},
            {QStringLiteral("platform"), QStringLiteral("Wii U")},
            {QStringLiteral("launchArguments"), QStringLiteral("-f -g \"{rom}\"")},
            {QStringLiteral("supportedExtensions"), QStringList{QStringLiteral(".wua"), QStringLiteral(".wux"), QStringLiteral(".wud"), QStringLiteral(".rpx")}}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("RetroArch")},
            {QStringLiteral("platform"), QStringLiteral("Multi-System")},
            {QStringLiteral("launchArguments"), QStringLiteral("-f -L cores/core_libretro.dll \"{rom}\"")},
            {QStringLiteral("supportedExtensions"), QStringList{QStringLiteral(".sfc"), QStringLiteral(".smc"), QStringLiteral(".gba"), QStringLiteral(".nes"), QStringLiteral(".n64"), QStringLiteral(".z64"), QStringLiteral(".cue"), QStringLiteral(".chd")}}
        }
    };
}

QStringList EmulatorManager::parseArgumentsTemplate(
    const QString &argTemplate,
    const QString &romPath,
    const QString &gameTitle) {
    const QFileInfo fi(romPath);
    const QString nativeRom = QDir::toNativeSeparators(romPath);
    const QString nativeDir = QDir::toNativeSeparators(fi.absolutePath());
    const QString romName = fi.completeBaseName();

    // Tokenize first respecting quotes, then substitute tokens inside each token
    // so paths with spaces stay cleanly in a single argv element!
    QStringList rawTokens;
    QString current;
    bool inQuotes = false;
    QChar quoteChar;

    for (int i = 0; i < argTemplate.size(); ++i) {
        const QChar c = argTemplate.at(i);
        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
            } else {
                current.append(c);
            }
        } else {
            if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
                inQuotes = true;
                quoteChar = c;
            } else if (c.isSpace()) {
                if (!current.isEmpty()) {
                    rawTokens.append(current);
                    current.clear();
                }
            } else {
                current.append(c);
            }
        }
    }
    if (!current.isEmpty()) {
        rawTokens.append(current);
    }

    QStringList expanded;
    expanded.reserve(rawTokens.size());
    for (QString tok : rawTokens) {
        tok.replace(QStringLiteral("{rom}"), nativeRom);
        tok.replace(QStringLiteral("{rom_dir}"), nativeDir);
        tok.replace(QStringLiteral("{rom_name}"), romName);
        tok.replace(QStringLiteral("{title}"), gameTitle.isEmpty() ? romName : gameTitle);
        expanded.append(tok);
    }
    return expanded;
}

} // namespace Aether
