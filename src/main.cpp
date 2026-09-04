// hyprroll-cpp — Qt6 + QML, reuses qml/* 1:1 from python prototype
// LayerShell overlay when HAS_LAYER_SHELL and config use_layer_shell
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>
#ifdef HAS_LAYER_SHELL
#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>
#endif

int main(int argc, char *argv[]) {
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);
    QGuiApplication app(argc, argv);
    app.setApplicationName("hyprroll");
    app.setDesktopFileName("hyprroll");
#ifdef HAS_LAYER_SHELL
    // LayerShell overlay center — waybar-like, not tiled
    // Actual window setup done in QML via LayerShellQt attached properties
    // when config.use_layer_shell true
    LayerShellQt::Shell::useLayerShell();
#endif
    QQmlApplicationEngine engine;
    // TODO: expose WallpaperModel, Config, Backend (port of backend/*.py)
    // For now loads QML directly — frontend ideally repeats python variant
    engine.load(QUrl::fromLocalFile(QStringLiteral("qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
