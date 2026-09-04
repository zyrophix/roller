// hyprroll-cpp — Qt6 + QML, reuses qml/* 1:1 from python prototype
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#ifdef HAS_LAYER_SHELL
#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>
#endif
#include "backend/Config.h"
#include "backend/Repository.h"
#include "backend/Metadata.h"
#include "backend/Model.h"
#include "backend/Backend.h"

static QJsonObject loadConfig(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    auto doc = QJsonDocument::fromJson(f.readAll());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

int main(int argc, char *argv[]) {
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);
    QGuiApplication app(argc, argv);
    app.setApplicationName("hyprroll");
    app.setDesktopFileName("hyprroll");
    // LayerShell is handled in QML via org.kde.layershell when use_layer_shell is true
    QString appDir = QCoreApplication::applicationDirPath();
    QString qmlPath = QDir(appDir).filePath("../qml/Main.qml");
    if (!QFile::exists(qmlPath)) qmlPath = QDir::current().filePath("qml/Main.qml");
    if (!QFile::exists(qmlPath)) qmlPath = QDir(appDir).filePath("../../qml/Main.qml");
    // Fallback to source dir
    if (!QFile::exists(qmlPath)) qmlPath = QStringLiteral("/home/you/Projects/hyprroll/qml/Main.qml");

    QString configPath = QDir::cleanPath(QDir(appDir).filePath("../config.json"));
    if (!QFile::exists(configPath)) configPath = QDir::current().filePath("config.json");
    if (!QFile::exists(configPath)) configPath = "/home/you/Projects/hyprroll/config.json";
    QJsonObject cfgObj = loadConfig(configPath);
    if (cfgObj.isEmpty()) {
        // defaults
        cfgObj["wallpaper_path"] = QDir::home().filePath("Pictures/Wallpapers");
        cfgObj["cache_path"] = QDir::home().filePath(".cache/hyprroll/thumbs");
        cfgObj["number_of_pictures"] = 5;
        cfgObj["border_color"] = "#b4befe";
    }
    QString wallpaperDir = cfgObj.value("wallpaper_path").toString(QDir::home().filePath("Pictures/Wallpapers"));
    wallpaperDir.replace("~", QDir::homePath());
    QString cacheDir = cfgObj.value("cache_path").toString(QDir::home().filePath(".cache/hyprroll/thumbs"));
    cacheDir.replace("~", QDir::homePath());
    QDir().mkpath(wallpaperDir);
    QDir().mkpath(cacheDir);

    Config cfg(cfgObj);
    Repository repo(wallpaperDir);
    repo.refresh();
    MetadataStore store(QDir(cacheDir).filePath("metadata.json"));
    store.load();
    repo.setMetadata(store.data);
    WallpaperModel model;
    model.setDirs(wallpaperDir, cacheDir);
    Backend backend(&repo, &store, &model, &cfg);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("wallpaperModel", &model);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("config", &cfg);
    engine.load(QUrl::fromLocalFile(qmlPath));
    if (engine.rootObjects().isEmpty()) return 1;
    backend.refresh();
    return app.exec();
}
