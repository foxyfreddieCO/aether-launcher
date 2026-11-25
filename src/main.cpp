#include "audio/AudioManager.h"
#include "core/AppPaths.h"
#include "core/Database.h"
#include "core/SettingsManager.h"
#include "emulators/EmulatorManager.h"
#include "emulators/GameLauncher.h"
#include "input/ControllerManager.h"
#include "library/LibraryManager.h"
#include "system/SystemIntegration.h"
#include "updater/UpdateService.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Aether"));
    app.setApplicationName(QStringLiteral("AetherLauncher"));
    app.setApplicationVersion(QString::fromLatin1(Aether::UpdateService::kAppVersion));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Aether Launcher - Windows 11 Liquid Glass Gaming Console Launcher"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringLiteral("update-healthcheck"), QStringLiteral("Internal updater boot healthcheck marker path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("data-dir"), QStringLiteral("Override user data directory"), QStringLiteral("dir")});
    parser.addOption({QStringLiteral("windowed"), QStringLiteral("Launch in windowed mode")});
    parser.process(app);

    const QString dataDirOverride = parser.value(QStringLiteral("data-dir"));
    const QString healthcheckMarker = parser.value(QStringLiteral("update-healthcheck"));

    Aether::AppPaths paths(dataDirOverride);
    if (!paths.ensureDirectoriesExist()) {
        qCritical("Failed to initialize user data directories at: %s", qPrintable(paths.userDataDir()));
        return 1;
    }

    Aether::Database database(paths.databaseFile());
    if (!database.open()) {
        qCritical("Failed to open library database: %s", qPrintable(database.lastError()));
        return 1;
    }

    Aether::SettingsManager settings(paths);
    settings.load();
    if (parser.isSet(QStringLiteral("windowed"))) {
        settings.setFullscreen(false);
    }

    Aether::EmulatorManager emulators(database);
    emulators.initializeDefaults();

    Aether::LibraryManager library(paths, database, emulators, settings);
    library.reload();

    if (settings.autoBackupOnLaunch() && library.totalGamesCount() > 0) {
        library.createBackupSnapshot();
    }

    Aether::GameLauncher launcher(database, emulators);
    QObject::connect(&launcher, &Aether::GameLauncher::sessionRecorded, &library, [&library]() {
        library.reload();
    });

    Aether::ControllerManager controller(settings);
    controller.initialize();

    Aether::AudioManager audio(paths, settings);
    audio.initialize();

    Aether::SystemIntegration sys;
    sys.startMonitoring();
    QObject::connect(&settings, &Aether::SettingsManager::startupWithWindowsChanged,
                     &sys, &Aether::SystemIntegration::setStartupWithWindows);

    Aether::UpdateService updater(paths, settings);
    if (settings.autoCheckUpdates() && !settings.updateManifestUrl().trimmed().isEmpty()) {
        QTimer::singleShot(3000, &updater, &Aether::UpdateService::checkForUpdates);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("emulators"), &emulators);
    engine.rootContext()->setContextProperty(QStringLiteral("library"), &library);
    engine.rootContext()->setContextProperty(QStringLiteral("launcher"), &launcher);
    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("audio"), &audio);
    engine.rootContext()->setContextProperty(QStringLiteral("sys"), &sys);
    engine.rootContext()->setContextProperty(QStringLiteral("updater"), &updater);

    const QUrl mainUrl(QStringLiteral("qrc:/qt/qml/AetherUI/src/ui/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [mainUrl, healthcheckMarker](QObject *obj, const QUrl &objUrl) {
                         if (!obj && mainUrl == objUrl) {
                             QCoreApplication::exit(-1);
                             return;
                         }
                         if (obj && !healthcheckMarker.isEmpty()) {
                             Aether::UpdateService::writeHealthcheckMarker(healthcheckMarker);
                         }
                     },
                     Qt::QueuedConnection);

    engine.load(mainUrl);
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
