#pragma once

#include "core/Database.h"
#include "core/Models.h"

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <optional>

namespace Aether {

class EmulatorManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList emulators READ emulatorsVariantList NOTIFY emulatorsChanged)
    Q_PROPERTY(QVariantMap lichiiProfile READ lichiiProfileVariant NOTIFY emulatorsChanged)

public:
    static constexpr const char *kLichiiId = "lichii-switch2";

    explicit EmulatorManager(Database &database, QObject *parent = nullptr);

    void initializeDefaults();
    void reload();

    QList<EmulatorProfile> emulators() const { return m_emulators; }
    QVariantList emulatorsVariantList() const;
    QVariantMap lichiiProfileVariant() const;

    std::optional<EmulatorProfile> findById(const QString &id) const;
    std::optional<EmulatorProfile> matchExtension(const QString &filePath) const;

    Q_INVOKABLE QString addEmulator(const QVariantMap &data);
    Q_INVOKABLE bool updateEmulator(const QString &id, const QVariantMap &data);
    Q_INVOKABLE bool removeEmulator(const QString &id);
    Q_INVOKABLE bool addGameDirectory(const QString &emulatorId, const QString &directory);
    Q_INVOKABLE bool removeGameDirectory(const QString &emulatorId, const QString &directory);
    Q_INVOKABLE QVariantList presetTemplates() const;

    // Expands template tokens ({rom}, {rom_dir}, {rom_name}, {title}) into argv list
    static QStringList parseArgumentsTemplate(
        const QString &argTemplate,
        const QString &romPath,
        const QString &gameTitle = QString());

    static QStringList normalizeExtensions(const QStringList &raw);

signals:
    void emulatorsChanged();

private:
    Database &m_db;
    QList<EmulatorProfile> m_emulators;
};

} // namespace Aether
