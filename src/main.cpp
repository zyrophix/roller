// roller — Qt6 + QML wallpaper picker
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QLockFile>
#include <QStandardPaths>
#include <cstdio>
#ifdef HAS_LAYER_SHELL
#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>
#endif
#include "backend/AppConfig.h"
#include "backend/WallpaperRepository.h"
#include "backend/WallpaperModel.h"
#include "backend/WallpaperFilterProxy.h"
#include "backend/ThumbnailCache.h"
#include "backend/PickerController.h"

#ifndef ROLLER_VERSION
#define ROLLER_VERSION "unknown"
#endif

// CLI output goes through printf rather than qInfo: qWarning/qInfo from this
// binary vanish when stderr is not a tty in this environment, and --help or
// a rejected second instance must never be silent.
static void printUsage() {
    std::fputs(
        "roller " ROLLER_VERSION " - keyboard-driven wallpaper picker (Wayland)\n"
        "\n"
        "Usage: roller [options]\n"
        "\n"
        "Options:\n"
        "  -h, --help           show this help and exit\n"
        "  -v, --version        print the version and exit\n"
        "      --allow-multiple do not enforce the single-instance lock\n"
        "\n"
        "Config is read from $XDG_CONFIG_HOME/roller/config.json, then\n"
        "../config.json next to the binary. The working directory is not\n"
        "searched, so a stray config.json cannot silently change behaviour.\n"
        "\n"
        "Backends (\"backend\" in config.json): auto, awww, swww, hyprpaper,\n"
        "waypaper, swaybg. Video files always go through mpvpaper.\n"
        "\n"
        "Keys: h/l or Left/Right step, d/u jump a page, / or Ctrl+F search,\n"
        "Enter apply, Esc close, wheel and drag to scroll, click to select or apply.\n"
        "\n"
        "LayerShell is required: roller is Wayland-only and will not start on X11.\n",
        stdout);
}

// Qt's default message handler produces nothing from this binary when stderr
// is not a tty, which makes every diagnostic invisible in a launcher log.
// Writing through stderr ourselves fixes it for our own warnings and for
// anything Qt itself reports.
static void rollerMessageHandler(QtMsgType type, const QMessageLogContext &ctx,
                                 const QString &msg) {
    Q_UNUSED(ctx);
    const char *level = "info";
    switch (type) {
    case QtDebugMsg:    level = "debug"; break;
    case QtInfoMsg:     level = "info";  break;
    case QtWarningMsg:  level = "warn";  break;
    case QtCriticalMsg: level = "crit";  break;
    case QtFatalMsg:    level = "fatal"; break;
    }
    std::fprintf(stderr, "roller: %s: %s\n", level, qPrintable(msg));
    std::fflush(stderr);
    if (type == QtFatalMsg) std::abort();
}

// Config search order:
//   1. $XDG_CONFIG_HOME/roller/config.json - the canonical location
//   2. ../config.json next to the binary - portable tree install, and what a
//      developer running build/roller from the repo gets
// The working directory is deliberately NOT searched. It used to be, and a
// config.json lying in whatever directory the launcher happened to start from
// silently changed the behaviour, which made it impossible to tell which
// settings were actually in effect.
static QStringList configSearchPaths() {
    QStringList out;
    out << QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
             .filePath("roller/config.json");
    const QString sibling =
        QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).filePath("../config.json"));
    if (!out.contains(sibling)) out << sibling;
    return out;
}

static QJsonObject loadConfig(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    auto doc = QJsonDocument::fromJson(f.readAll());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

int main(int argc, char *argv[]) {
    bool allowMultiple = false;
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a == u8"-h" || a == u8"--help") { printUsage(); return 0; }
        if (a == u8"-v" || a == u8"--version") {
            std::printf("roller %s\n", ROLLER_VERSION);
            return 0;
        }
        if (a == u8"--allow-multiple") { allowMultiple = true; continue; }
        std::fprintf(stderr, "roller: unknown option %s - try --help\n",
                     qPrintable(a));
        return 1;
    }

    // Checked before QGuiApplication: this is a compile-time constant, and
    // constructing a QGuiApplication on a machine with no display aborts with
    // Qt's own platform error, which hides the explanation entirely. --version
    // and --help already returned above, so they still work.
#ifndef HAS_LAYER_SHELL
    std::fprintf(stderr,
                 "roller: built without layer-shell-qt, so it cannot open a layer "
                 "surface. This build is for CI and is not usable.\n");
    return 1;
#endif

    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);
    qInstallMessageHandler(rollerMessageHandler);
    QGuiApplication app(argc, argv);
    app.setApplicationName("roller");
    app.setDesktopFileName("roller");

    // Single instance. Two rollers would each hold a layer surface with
    // exclusive keyboard, each start a thumbnail worker writing the same
    // cache files, and each write the current-wallpaper state file. The lock
    // is taken before the QML engine and before generateMissing, so a
    // rejected instance never touches the cache.
    //
    // QLockFile rather than QLocalServer: a local socket file survives a
    // crash and then blocks the next start, and the usual "delete it and
    // claim it" recovery lets a second instance steal the channel from a
    // live one. QLockFile decides from pid plus process name, so an instance
    // that was killed is detected and its lock reclaimed.
    QLockFile lock(QDir(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation))
                       .filePath("roller.lock"));
    if (!allowMultiple) {
        // Qt requires setStaleLockTime(0) for a resource held a long time:
        // the 30 second default would declare our own live lock stale.
        lock.setStaleLockTime(0);
        if (!lock.tryLock(0)) {
            if (lock.error() == QLockFile::LockFailedError) {
                qint64 pid = 0;
                QString host, appname;
                lock.getLockInfo(&pid, &host, &appname);
                std::fprintf(stderr,
                             "roller is already running (pid %lld), not starting again\n",
                             static_cast<long long>(pid));
                return 0;
            }
            std::fprintf(stderr, "roller: cannot acquire the single-instance lock (error %d)\n",
                         static_cast<int>(lock.error()));
            return 1;
        }
    }

    // LayerShell is handled in QML via org.kde.layershell
    QString configPath;
    for (const QString &candidate : configSearchPaths()) {
        if (QFile::exists(candidate)) { configPath = candidate; break; }
    }
    QJsonObject cfgObj = configPath.isEmpty() ? QJsonObject{} : loadConfig(configPath);
    if (configPath.isEmpty()) {
        // never resolve silently: a missing or ignored config is the reason
        // a setting appears to have no effect
        std::fprintf(stderr, "roller: no config found, using built-in defaults "
                             "(looked in %s)\n",
                     qPrintable(configSearchPaths().join(", ")));
        cfgObj["wallpaper_path"] = QDir::home().filePath("Pictures/Wallpapers");
        cfgObj["cache_path"] = QDir::home().filePath(".cache/roller/thumbs");
        cfgObj["number_of_pictures"] = 5;
        cfgObj["border_color"] = "#b4befe";
    } else if (cfgObj.isEmpty()) {
        std::fprintf(stderr, "roller: %s is not valid JSON, using built-in defaults\n",
                     qPrintable(configPath));
        cfgObj["wallpaper_path"] = QDir::home().filePath("Pictures/Wallpapers");
        cfgObj["cache_path"] = QDir::home().filePath(".cache/roller/thumbs");
        cfgObj["number_of_pictures"] = 5;
        cfgObj["border_color"] = "#b4befe";
    } else {
        std::fprintf(stderr, "roller: config %s\n", qPrintable(configPath));
    }
    auto expandPath = [](QString p) {
        if (p.startsWith("~/")) p.replace(0, 1, QDir::homePath());
        else if (p == "~") p = QDir::homePath();
        return p;
    };
    QString wallpaperDir = expandPath(cfgObj.value("wallpaper_path").toString(QDir::home().filePath("Pictures/Wallpapers")));
    QString cacheDir = expandPath(cfgObj.value("cache_path").toString(QDir::home().filePath(".cache/roller/thumbs")));
    // an empty grid is otherwise indistinguishable from a wrong path
    if (!QDir().mkpath(wallpaperDir))
        qWarning() << "roller: cannot create wallpaper_path" << wallpaperDir;
    if (!QDir().mkpath(cacheDir))
        qWarning() << "roller: cannot create cache_path" << cacheDir;

    AppConfig cfg(cfgObj);
    cfg.setStateFile(QDir(QFileInfo(cacheDir).absolutePath()).filePath(".current"));
    WallpaperRepository repo(wallpaperDir);
    repo.setVideoExtensions(cfg.videoExtensions());
    repo.refresh();
    WallpaperModel sourceModel;
    sourceModel.setDirs(wallpaperDir, cacheDir);
    WallpaperFilterProxy proxyModel;
    proxyModel.setSource(&sourceModel);
    ThumbnailCache thumbCache(cacheDir);
    thumbCache.setTargetHeight(cfg.thumbnailHeight());
    thumbCache.setMaxBytes(qint64(cfg.cacheMaxMb()) * 1024 * 1024);
    QObject::connect(&thumbCache, &ThumbnailCache::thumbReady, &sourceModel, &WallpaperModel::onThumbReady);
    PickerController backend(&repo, &sourceModel, &proxyModel, &cfg);
    // new or removed wallpapers need their thumbs too, not only at startup
    QObject::connect(&backend, &PickerController::libraryRescanned, &backend, [&]{
        thumbCache.generateMissing(repo.getAll());
    });

    const QString appDir = QCoreApplication::applicationDirPath();
    QQmlApplicationEngine engine;
    // two index domains are exposed to QML: wallpaperModel is the filtered
    // proxy, sourceWallpaperModel is the unfiltered list. Calling get_*_at
    // on the source with a proxy index would show or apply the wrong file
    // while a filter is active.
    engine.rootContext()->setContextProperty("wallpaperModel", &proxyModel);
    engine.rootContext()->setContextProperty("sourceWallpaperModel", &sourceModel);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("config", &cfg);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Roller/qml/Main.qml")));
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
