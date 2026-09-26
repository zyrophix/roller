#pragma once
#include <QString>
#include <QStringList>

// Static-image wallpaper backends. `swww` is the former name of `awww`, so
// both resolve to the same binary. There is deliberately no X11 backend: roller
// requires a wlr-layer-shell compositor, and none of those render an X11 root
// window as a desktop, so such a backend could only ever silently do nothing.
enum class WallpaperBackend {
    Awww, Hyprpaper, Waypaper, Swaybg
};

// Backends in the order `auto` tries them.
QList<WallpaperBackend> availableBackends();
// Resolve a config value ("auto", "awww", "swww", "hyprpaper", "waypaper",
// "swaybg"). An unknown or unavailable name falls back to the first available
// backend, and `resolved` is set to the canonical name so QML can tell the
// user what actually got used.
WallpaperBackend resolveBackend(const QString &name, QString *resolved = nullptr);
QString backendName(WallpaperBackend b);

// Video files are always played through mpvpaper; none of the static-image
// backends can render them.
QStringList defaultVideoExtensions();
bool isVideoFile(const QString &path, const QStringList &extensions);

// Output of the focused monitor, via hyprctl then swaymsg. Empty if neither
// compositor answers.
QString focusedMonitor();

struct ApplyResult {
    bool ok = false;
    QString backend;      // canonical name actually used
    QString error;        // human readable reason when !ok
};

// Everything one apply needs. A parameter list this long made the call site in
// PickerController unreadable and hid which values were actually optional.
struct WallpaperRequest {
    QString path;
    WallpaperBackend backend = WallpaperBackend::Awww;
    QString transitionType;
    QString transitionPos;
    double transitionDuration = 1.2;
    int transitionFps = 60;
    QStringList videoExtensions;
    // mpv --hwdec value. Empty leaves mpv's own default (no hw decoding).
    QString videoHwdec;
    QString stableCopyPath;
    QString postApplyCommand;
    QString wallpaperDir;
};

ApplyResult applyWallpaper(const WallpaperRequest &req);
