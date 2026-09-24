#include "WallpaperBackend.h"
#include <QStandardPaths>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

namespace {

struct BackendSpec {
    WallpaperBackend backend;
    const char *name;
    const char *binary;   // empty when it is not a plain executable
};

const BackendSpec kSpecs[] = {
    { WallpaperBackend::Awww,      "awww",      "awww" },
    { WallpaperBackend::Hyprpaper, "hyprpaper", "hyprctl" },
    { WallpaperBackend::Waypaper,  "waypaper",  "waypaper" },
    { WallpaperBackend::Swaybg,    "swaybg",    "swaybg" },
    { WallpaperBackend::Feh,       "feh",       "feh" },
    { WallpaperBackend::Mlw4,      "ml4w",      "" },
};

const BackendSpec &specFor(WallpaperBackend b) {
    for (const auto &s : kSpecs)
        if (s.backend == b) return s;
    return kSpecs[0];
}

// swww is the old name of awww and is installed as either one
QString findAwww() {
    for (const char *n : {"awww", "swww"}) {
        const QString p = QStandardPaths::findExecutable(QString::fromLatin1(n));
        if (!p.isEmpty()) return p;
    }
    return {};
}

QString binaryPath(WallpaperBackend b) {
    switch (b) {
    case WallpaperBackend::Awww:      return findAwww();
    case WallpaperBackend::Hyprpaper: return QStandardPaths::findExecutable("hyprctl");
    case WallpaperBackend::Waypaper:  return QStandardPaths::findExecutable("waypaper");
    case WallpaperBackend::Swaybg:    return QStandardPaths::findExecutable("swaybg");
    case WallpaperBackend::Feh:       return QStandardPaths::findExecutable("feh");
    case WallpaperBackend::Mlw4:      return {};   // user script, checked separately
    }
    return {};
}

QString ml4wScript() {
    return QDir::home().filePath(".config/ml4w/scripts/ml4w-wallpaper");
}

bool backendUsable(WallpaperBackend b) {
    if (b == WallpaperBackend::Mlw4) return QFileInfo::exists(ml4wScript());
    return !binaryPath(b).isEmpty();
}

const QSet<QString> kOkTransitions = {
    "none", "simple", "fade", "left", "right", "top", "bottom",
    "center", "outer", "any", "grow", "wipe", "wave"
};

bool run(const QString &bin, const QStringList &args, QString *error) {
    if (bin.isEmpty()) { if (error) *error = "backend not found"; return false; }
    if (!QProcess::startDetached(bin, args)) {
        if (error) *error = QStringLiteral("failed to start %1").arg(QFileInfo(bin).fileName());
        return false;
    }
    return true;
}

// swaybg is a foreground process, so the previous instance has to go first
static qint64 s_swaybgPid = 0;
void killPreviousSwaybg() {
    if (s_swaybgPid <= 0) return;
    QProcess::startDetached(QStringLiteral("kill"),
                            {QStringLiteral("-TERM"), QString::number(s_swaybgPid)});
    s_swaybgPid = 0;
}

QString runAndCapture(const QString &bin, const QStringList &args, int timeoutMs = 2000) {
    QProcess p;
    p.start(bin, args);
    if (!p.waitForStarted(timeoutMs)) return {};
    if (!p.waitForFinished(timeoutMs)) { p.kill(); return {}; }
    return QString::fromUtf8(p.readAllStandardOutput());
}

} // namespace

QList<WallpaperBackend> availableBackends() {
    QList<WallpaperBackend> out;
    for (const auto &s : kSpecs)
        if (backendUsable(s.backend)) out.append(s.backend);
    return out;
}

QString backendName(WallpaperBackend b) { return QString::fromLatin1(specFor(b).name); }

WallpaperBackend resolveBackend(const QString &name, QString *resolved) {
    const QString n = name.trimmed().toLower();
    auto finish = [&](WallpaperBackend b) {
        if (resolved) *resolved = backendName(b);
        return b;
    };
    if (n == "auto" || n.isEmpty()) {
        const auto list = availableBackends();
        return finish(list.isEmpty() ? WallpaperBackend::Awww : list.first());
    }
    // swww is the former name of awww
    if (n == "swww") return finish(WallpaperBackend::Awww);
    for (const auto &s : kSpecs)
        if (n == s.name) {
            if (backendUsable(s.backend)) return finish(s.backend);
            const auto list = availableBackends();
            return finish(list.isEmpty() ? WallpaperBackend::Awww : list.first());
        }
    const auto list = availableBackends();
    return finish(list.isEmpty() ? WallpaperBackend::Awww : list.first());
}

QStringList defaultVideoExtensions() {
    return { "mp4", "webm", "mov", "avi", "mkv", "gif", "m4v",
             "flv", "wmv", "mpeg", "3gp" };
}

bool isVideoFile(const QString &path, const QStringList &extensions) {
    const QStringList exts = extensions.isEmpty() ? defaultVideoExtensions() : extensions;
    const QString suffix = QFileInfo(path).suffix().toLower();
    for (const auto &e : exts)
        if (suffix == e.trimmed().toLower()) return true;
    return false;
}

QString focusedMonitor() {
    if (!QStandardPaths::findExecutable("hyprctl").isEmpty()) {
        const QString json = runAndCapture(QStringLiteral("hyprctl"), {"monitors", "-j"});
        if (!json.isEmpty()) {
            const auto doc = QJsonDocument::fromJson(json.toUtf8());
            if (doc.isArray()) {
                const auto arr = doc.array();
                // prefer the focused monitor, then the first one reported
                for (const auto &v : arr) {
                    const auto o = v.toObject();
                    if (o.value("focused").toBool())
                        return o.value("name").toString();
                }
                if (!arr.isEmpty())
                    return arr.first().toObject().value("name").toString();
            }
        }
    }
    if (!QStandardPaths::findExecutable("swaymsg").isEmpty()) {
        const QString json = runAndCapture(QStringLiteral("swaymsg"), {"-t", "get_outputs"});
        if (!json.isEmpty()) {
            const auto doc = QJsonDocument::fromJson(json.toUtf8());
            if (doc.isArray() && !doc.array().isEmpty())
                return doc.array().first().toObject().value("name").toString();
        }
    }
    return {};
}

ApplyResult applyWallpaper(const QString &path,
                           WallpaperBackend backend,
                           const QString &transitionType,
                           const QString &transitionPos,
                           double transitionDuration,
                           int transitionFps,
                           const QStringList &videoExtensions,
                           const QString &stableCopyPath,
                           const QString &postApplyCommand) {
    ApplyResult r;
    r.backend = backendName(backend);
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        r.error = QStringLiteral("wallpaper not found");
        return r;
    }

    // Video always goes through mpvpaper, whatever the configured backend is
    if (isVideoFile(path, videoExtensions)) {
        const QString mpvpaper = QStandardPaths::findExecutable("mpvpaper");
        if (mpvpaper.isEmpty()) {
            r.error = QStringLiteral("mpvpaper is required for video wallpapers");
            return r;
        }
        const QString monitor = focusedMonitor();
        if (monitor.isEmpty()) {
            r.error = QStringLiteral("cannot determine monitor output");
            return r;
        }
        r.backend = QStringLiteral("mpvpaper");
        // -f forks into the background itself, no trailing '&' needed
        r.ok = run(mpvpaper, {"-f", "-o", "no-audio loop", monitor, path}, &r.error);
        return r;
    }

    switch (backend) {
    case WallpaperBackend::Awww: {
        const QString bin = findAwww();
        if (bin.isEmpty()) { r.error = QStringLiteral("awww not found"); return r; }
        // never pass raw config text as flags
        const QString t = kOkTransitions.contains(transitionType) ? transitionType
                                                                  : QStringLiteral("grow");
        const QStringList xy = transitionPos.split(',');
        bool okX = false, okY = false;
        const double x = xy.value(0).toDouble(&okX), y = xy.value(1).toDouble(&okY);
        const QString pos = (okX && okY) ? QStringLiteral("%1,%2").arg(x).arg(y)
                                         : QStringLiteral("0.5,0.5");
        const double d = std::isfinite(transitionDuration)
                       ? qBound(0.0, transitionDuration, 10.0) : 1.2;
        const int f = transitionFps > 0 ? qMin(transitionFps, 240) : 60;
        r.ok = run(bin, {"img", path,
                         "--transition-type", t,
                         "--transition-pos", pos,
                         "--transition-duration", QString::number(d),
                         "--transition-fps", QString::number(f)}, &r.error);
        break;
    }
    case WallpaperBackend::Hyprpaper: {
        const QString hyprctl = binaryPath(backend);
        if (hyprctl.isEmpty()) { r.error = QStringLiteral("hyprctl not found"); return r; }
        // preload first so the wallpaper appears without a decode stall
        if (!run(hyprctl, {"hyprpaper", "preload", path}, &r.error)) return r;
        r.ok = run(hyprctl, {"hyprpaper", "wallpaper", QChar(',') + path}, &r.error);
        break;
    }
    case WallpaperBackend::Waypaper:
        r.ok = run(binaryPath(backend), {"--wallpaper", path}, &r.error);
        break;
    case WallpaperBackend::Swaybg: {
        const QString bin = binaryPath(backend);
        if (bin.isEmpty()) { r.error = QStringLiteral("swaybg not found"); return r; }
        killPreviousSwaybg();
        // -i drops to the background on its own, like the reference dispatcher
        if (!run(bin, {"-i", path, "-m", "fill"}, &r.error)) return r;
        r.ok = true;
        break;
    }
    case WallpaperBackend::Feh:
        // X11 / XWayland only, not truly Wayland-native
        r.ok = run(binaryPath(backend), {"--bg-fill", path}, &r.error);
        break;
    case WallpaperBackend::Mlw4: {
        const QString script = ml4wScript();
        if (!QFileInfo::exists(script)) {
            r.error = QStringLiteral("ml4w wallpaper script not found");
            return r;
        }
        r.ok = run(script, {path}, &r.error);
        break;
    }
    }

    if (!r.ok) return r;

    // publish the wallpaper at a fixed path for lockscreens, bars, themes
    if (!stableCopyPath.trimmed().isEmpty()) {
        QString target = stableCopyPath.trimmed();
        if (target.startsWith(QLatin1String("~/")))
            target.replace(0, 1, QDir::homePath());
        QDir().mkpath(QFileInfo(target).absolutePath());
        QFile::remove(target);
        QFile::copy(path, target);
    }

    // optional user hook, for wallust/pywal or reloading bars
    if (!postApplyCommand.trimmed().isEmpty()) {
        const QString sh = QStandardPaths::findExecutable("sh");
        if (!sh.isEmpty())
            QProcess::startDetached(sh, {"-c", postApplyCommand});
    }

    return r;
}
