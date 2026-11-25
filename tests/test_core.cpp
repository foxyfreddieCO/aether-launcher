#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/AppPaths.h"
#include "core/Database.h"
#include "core/SettingsManager.h"
#include "emulators/EmulatorManager.h"
#include "emulators/GameLauncher.h"
#include "library/LibraryManager.h"
#include "library/MetadataService.h"
#include "updater/UpdateService.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>

using namespace Aether;

TEST_CASE("AppPaths ensures isolated user data directories and validates path containment", "[core]") {
    QTemporaryDir temp;
    REQUIRE(temp.isValid());

    AppPaths paths(temp.path() + QStringLiteral("/userdata"));
    REQUIRE(paths.ensureDirectoriesExist());
    REQUIRE(QDir(paths.userDataDir()).exists());
    REQUIRE(QDir(paths.coversDir()).exists());
    REQUIRE(QDir(paths.backgroundsDir()).exists());
    REQUIRE(QDir(paths.backupsDir()).exists());
    REQUIRE(QDir(paths.updatesStagingDir()).exists());

    REQUIRE(AppPaths::isPathInsideDirectory(paths.userDataDir(), paths.databaseFile()));
    REQUIRE_FALSE(AppPaths::isPathInsideDirectory(paths.userDataDir(), paths.userDataDir() + QStringLiteral("/../escape.txt")));
    REQUIRE(AppPaths::sanitizeFilename(QStringLiteral("My:Game/Name*?")) == QStringLiteral("My_Game_Name_"));
}

TEST_CASE("Database persists games, emulators, collections, play sessions, and creates atomic backups", "[database]") {
    QTemporaryDir temp;
    REQUIRE(temp.isValid());

    Database db(temp.path() + QStringLiteral("/library.sqlite"));
    REQUIRE(db.open());

    GameEntry g;
    g.id = QStringLiteral("game-001");
    g.title = QStringLiteral("Starlight Odyssey");
    g.platform = QStringLiteral("Nintendo Switch 2");
    g.category = QStringLiteral("Adventure");
    g.executablePath = QStringLiteral("/roms/starlight.nsp");
    g.emulatorId = QString::fromLatin1(EmulatorManager::kLichiiId);
    g.isFavorite = true;
    g.dateAdded = QDateTime::currentDateTimeUtc();

    REQUIRE(db.upsertGame(g));
    auto loaded = db.getGame(g.id);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->title == QStringLiteral("Starlight Odyssey"));
    REQUIRE(loaded->isFavorite);

    const QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-185);
    const QDateTime end = QDateTime::currentDateTimeUtc();
    REQUIRE(db.recordPlaySession(g.id, 185, start, end));

    loaded = db.getGame(g.id);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->playtimeSeconds == 185);
    REQUIRE(loaded->playCount == 1);
    REQUIRE(loaded->lastPlayed.isValid());
    REQUIRE(db.totalPlaytimeSeconds() == 185);
    REQUIRE(db.totalSessionsCount() == 1);

    CollectionEntry col;
    col.id = QStringLiteral("col-1");
    col.name = QStringLiteral("Weekend Favorites");
    col.gameIds = {g.id};
    REQUIRE(db.upsertCollection(col));
    REQUIRE(db.getAllCollections().size() == 1);
    REQUIRE(db.getAllCollections().first().gameIds.contains(g.id));

    const QString backupFile = temp.path() + QStringLiteral("/backup.sqlite");
    REQUIRE(db.createBackup(backupFile));
    REQUIRE(QFile::exists(backupFile));
}

TEST_CASE("SettingsManager persists appearance, custom themes, widgets, and button mappings", "[settings]") {
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    AppPaths paths(temp.path());
    REQUIRE(paths.ensureDirectoriesExist());

    {
        SettingsManager s(paths);
        REQUIRE(s.load());
        s.setAccentColor(QStringLiteral("#2EE6B8"));
        s.setGlassOpacity(0.35);
        s.setBlurIntensity(0.88);
        s.setAnimationSpeed(1.4);
        s.setButtonMapping(QStringLiteral("confirm"), QStringLiteral("east"));
        s.setWidgetEnabled(QStringLiteral("wifi"), false);
        s.setWidgetSize(QStringLiteral("clock"), QStringLiteral("large"));
        s.moveWidget(QStringLiteral("clock"), 1);
        const QString customId = s.saveCustomTheme(QStringLiteral("Midnight Mint"));
        REQUIRE_FALSE(customId.isEmpty());
    }

    // Reload from disk in fresh instance
    SettingsManager s2(paths);
    REQUIRE(s2.load());
    REQUIRE(s2.accentColor() == QStringLiteral("#2EE6B8"));
    REQUIRE(s2.glassOpacity() == 0.35);
    REQUIRE(s2.buttonMappings().value(QStringLiteral("confirm")).toString() == QStringLiteral("east"));

    bool foundCustomTheme = false;
    for (const QVariant &v : s2.themesVariantList()) {
        if (v.toMap().value(QStringLiteral("name")).toString() == QStringLiteral("Midnight Mint")) {
            foundCustomTheme = true;
        }
    }
    REQUIRE(foundCustomTheme);
}

TEST_CASE("EmulatorManager initializes dedicated configurable Lichii profile and expands arguments safely", "[emulators]") {
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    Database db(temp.path() + QStringLiteral("/emu.sqlite"));
    REQUIRE(db.open());

    EmulatorManager emu(db);
    emu.initializeDefaults();

    auto lichii = emu.findById(QString::fromLatin1(EmulatorManager::kLichiiId));
    REQUIRE(lichii.has_value());
    REQUIRE(lichii->isDedicatedLichii);
    REQUIRE(lichii->platform == QStringLiteral("Nintendo Switch 2"));
    // Must not hardcode undocumented installation paths
    REQUIRE(lichii->executablePath.isEmpty());
    // Dedicated Lichii profile cannot be accidentally deleted
    REQUIRE_FALSE(emu.removeEmulator(lichii->id));

    const QStringList args = EmulatorManager::parseArgumentsTemplate(
        QStringLiteral("--fullscreen \"{rom}\" --name \"{rom_name}\""),
        QStringLiteral("/Users/test/Switch 2 Games/Super Adventure [0100ABCD].nsp"),
        QStringLiteral("Super Adventure"));
    REQUIRE(args.size() == 4);
    REQUIRE(args[0] == QStringLiteral("--fullscreen"));
    REQUIRE(args[1].endsWith(QStringLiteral("Super Adventure [0100ABCD].nsp")));
    REQUIRE(args[2] == QStringLiteral("--name"));
    REQUIRE(args[3] == QStringLiteral("Super Adventure [0100ABCD]"));
}

TEST_CASE("LibraryManager scans directories, cleans ROM titles, filters, backs up, and exports/imports bundles", "[library]") {
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    AppPaths paths(temp.path() + QStringLiteral("/data"));
    REQUIRE(paths.ensureDirectoriesExist());

    Database db(paths.databaseFile());
    REQUIRE(db.open());
    SettingsManager settings(paths);
    settings.load();
    EmulatorManager emu(db);
    emu.initializeDefaults();
    LibraryManager lib(paths, db, emu, settings);
    lib.reload();

    // Create sample directory with ROMs and adjacent cover image
    const QString romDir = temp.path() + QStringLiteral("/roms");
    QDir().mkpath(romDir);
    QFile rom1(romDir + QStringLiteral("/Metroid_Prime_4_(USA)_[01001234].nsp"));
    REQUIRE(rom1.open(QIODevice::WriteOnly));
    rom1.write("dummy");
    rom1.close();

    QFile cover1(romDir + QStringLiteral("/Metroid_Prime_4_(USA)_[01001234]-cover.png"));
    REQUIRE(cover1.open(QIODevice::WriteOnly));
    cover1.write("pngdata");
    cover1.close();

    QFile exe1(romDir + QStringLiteral("/Celeste.exe"));
    REQUIRE(exe1.open(QIODevice::WriteOnly));
    exe1.write("exe");
    exe1.close();

    REQUIRE(MetadataService::cleanTitleFromFilename(rom1.fileName()) == QStringLiteral("Metroid Prime 4"));

    const int added = lib.scanDirectory(romDir);
    REQUIRE(added == 2);
    REQUIRE(lib.totalGamesCount() == 2);

    // Re-scanning same directory must not duplicate games
    REQUIRE(lib.scanDirectory(romDir) == 0);
    REQUIRE(lib.totalGamesCount() == 2);

    // Filter by platform
    lib.setPlatformFilter(QStringLiteral("Nintendo Switch 2"));
    REQUIRE(lib.filteredGames().size() == 1);
    REQUIRE(lib.filteredGames().first().toMap().value(QStringLiteral("title")).toString() == QStringLiteral("Metroid Prime 4"));
    REQUIRE_FALSE(lib.filteredGames().first().toMap().value(QStringLiteral("coverPath")).toString().isEmpty());

    lib.setPlatformFilter(QString());
    const QString bundlePath = temp.path() + QStringLiteral("/bundle.json");
    REQUIRE(lib.exportConfigurationBundle(bundlePath));
    REQUIRE(QFile::exists(bundlePath));

    const QString backupDir = lib.createBackupSnapshot();
    REQUIRE_FALSE(backupDir.isEmpty());
    REQUIRE(QFile::exists(backupDir + QStringLiteral("/library.sqlite")));
}

TEST_CASE("GameLauncher resolves commands, handles missing paths gracefully, and tracks real process playtime", "[launcher]") {
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    Database db(temp.path() + QStringLiteral("/launch.sqlite"));
    REQUIRE(db.open());
    EmulatorManager emu(db);
    emu.initializeDefaults();
    GameLauncher launcher(db, emu);

    // Missing executable -> graceful validation failure
    GameEntry badGame;
    badGame.id = QStringLiteral("bad-1");
    badGame.title = QStringLiteral("Broken Game");
    badGame.executablePath = temp.path() + QStringLiteral("/does_not_exist.exe");
    const auto badCmd = launcher.resolveCommand(badGame);
    REQUIRE_FALSE(badCmd.valid);
    REQUIRE(badCmd.errorReason.contains(QStringLiteral("not found")));

    // Real quick process launch test (/bin/echo on macOS/Unix, cmd.exe on Windows)
    GameEntry okGame;
    okGame.id = QStringLiteral("real-1");
    okGame.title = QStringLiteral("Quick Test Process");
#ifdef Q_OS_WIN
    okGame.executablePath = QStringLiteral("C:/Windows/System32/cmd.exe");
    okGame.customArguments = QStringLiteral("/c echo hello");
#else
    okGame.executablePath = QStringLiteral("/bin/echo");
    okGame.customArguments = QStringLiteral("hello");
#endif
    REQUIRE(db.upsertGame(okGame));

    bool exitedSignalReceived = false;
    QEventLoop loop;
    QObject::connect(&launcher, &GameLauncher::gameExited, &loop,
                     [&](const QString &gid, const QString &, qint64 secs, int code) {
                         REQUIRE(gid == okGame.id);
                         REQUIRE(secs >= 1);
                         REQUIRE(code == 0);
                         exitedSignalReceived = true;
                         loop.quit();
                     });
    QTimer::singleShot(4000, &loop, &QEventLoop::quit);

    REQUIRE(launcher.launchGame(okGame.id));
    loop.exec();

    REQUIRE(exitedSignalReceived);
    const auto updated = db.getGame(okGame.id);
    REQUIRE(updated.has_value());
    REQUIRE(updated->playCount == 1);
    REQUIRE(updated->playtimeSeconds >= 1);
}

TEST_CASE("UpdateService compares semver versions and verifies SHA-256 package integrity", "[updater]") {
    REQUIRE(UpdateService::compareVersions(QStringLiteral("1.1.0"), QStringLiteral("1.0.0")) > 0);
    REQUIRE(UpdateService::compareVersions(QStringLiteral("v1.0.0"), QStringLiteral("1.0.0")) == 0);
    REQUIRE(UpdateService::compareVersions(QStringLiteral("1.0.9"), QStringLiteral("1.0.10")) < 0);

    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    const QString file = temp.path() + QStringLiteral("/pkg.zip");
    QFile f(file);
    REQUIRE(f.open(QIODevice::WriteOnly));
    f.write("aether-package-bytes");
    f.close();

    const QString hash = UpdateService::computeFileSha256(file);
    REQUIRE(hash.size() == 64);
    REQUIRE(UpdateService::verifySha256(file, hash));
    REQUIRE_FALSE(UpdateService::verifySha256(file, QStringLiteral("0000000000000000000000000000000000000000000000000000000000000000")));

    const QString marker = temp.path() + QStringLiteral("/health.ok");
    REQUIRE(UpdateService::writeHealthcheckMarker(marker));
    REQUIRE(QFile::exists(marker));
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("AetherTests"));
    app.setApplicationName(QStringLiteral("AetherCoreTests"));
    return Catch::Session().run(argc, argv);
}
