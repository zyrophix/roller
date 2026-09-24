#pragma once
#include <QString>
#include <QStringList>

// Static-image wallpaper backends. `swww` is the former name of `awww`, so
// both resolve to the same binary.
enum class WallpaperBackend {
    Awww, Hyprpaper, Waypaper, Swaybg, Feh
};

// Backends in the order `auto` tries them.
QList<WallpaperBackend> availableBackends();
// Resolve a config value ("auto", "awww", "swww", "hyprpaper", "waypaper",
// "swaybg", "feh"). An unknown or unavailable name falls back to the first
// available backend, and `resolved` is set to the canonical name so QML can
// tell the user what actually got used.
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

ApplyResult applyWallpaper(const QString &path,
                           WallpaperBackend backend,
                           const QString &transitionType,
                           const QString &transitionPos,
                           double transitionDuration,
                           int transitionFps,
                           const QStringList &videoExtensions,
                           const QString &stableCopyPath,
                           const QString &postApplyCommand);
