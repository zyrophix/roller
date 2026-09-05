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
    QString configPath = QDir::cleanPath(QDir(appDir).filePath("../config.json"));
    if (!QFile::exists(configPath)) configPath = QDir::current().filePath("config.json");
    if (!QFile::exists(configPath)) configPath = QDir::home().filePath(".config/hyprroll/config.json");
    QJsonObject cfgObj = loadConfig(configPath);
    if (cfgObj.isEmpty()) {
        // defaults
        cfgObj["wallpaper_path"] = QDir::home().filePath("Pictures/Wallpapers");
        cfgObj["cache_path"] = QDir::home().filePath(".cache/hyprroll/thumbs");
        cfgObj["number_of_pictures"] = 5;
        cfgObj["border_color"] = "#b4befe";
    }
    auto expandPath = [](QString p) {
        if (p.startsWith("~/")) p.replace(0, 1, QDir::homePath());
        else if (p == "~") p = QDir::homePath();
        return p;
    };
    QString wallpaperDir = expandPath(cfgObj.value("wallpaper_path").toString(QDir::home().filePath("Pictures/Wallpapers")));
    QString cacheDir = expandPath(cfgObj.value("cache_path").toString(QDir::home().filePath(".cache/hyprroll/thumbs")));
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
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Hyprroll/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        QStringList fallbacks = {
            QDir(appDir).filePath("../qml/Main.qml"),
            QDir(appDir).filePath("../../qml/Main.qml"),
            QDir::current().filePath("qml/Main.qml")
        };
        for (const QString &p : fallbacks) {
            if (QFile::exists(p)) {
                engine.load(QUrl::fromLocalFile(p));
                if (!engine.rootObjects().isEmpty()) break;
            }
        }
    }
    if (engine.rootObjects().isEmpty()) return 1;
    backend.refresh();
    return app.exec();
}
