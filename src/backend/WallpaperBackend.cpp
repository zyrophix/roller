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
    }
    return {};
}

bool backendUsable(WallpaperBackend b) {
    return !binaryPath(b).isEmpty();
}

const QSet<QString> kOkTransitions = {
    "none", "simple", "fade", "left", "right", "top", "bottom",
    "center", "outer", "any", "grow", "wipe", "wave"
};

bool run(const QString &bin, const QStringList &args, QString *error, qint64 *pid = nullptr) {
    if (bin.isEmpty()) { if (error) *error = "backend not found"; return false; }
    qint64 started = 0;
    // the third parameter is workingDirectory; pid comes fourth
    const bool ok = pid ? QProcess::startDetached(bin, args, QString(), &started)
                        : QProcess::startDetached(bin, args);
    if (!ok) {
        if (error) *error = QStringLiteral("failed to start %1").arg(QFileInfo(bin).fileName());
        return false;
    }
    if (pid) *pid = started;
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

// same for mpvpaper, see the video branch for why -f must not be used
// A running video wallpaper has to be stopped by any later apply, not just by
// another video. mpvpaper loops forever and its layer outlives the change
// awww makes underneath, so applying a still image left the video on screen
// with no way back off it.
//
// Matching is scoped to files under wallpaperDir on purpose: that reclaims a
// video wallpaper left behind by an earlier roller run, whose pid this
// process never knew, while leaving an mpvpaper the user started for some
// unrelated purpose alone.
void stopVideoWallpaper(const QString &wallpaperDir) {
    if (wallpaperDir.isEmpty()) return;
    const QString prefix = QDir(wallpaperDir).absolutePath() + QLatin1Char('/');
    const QString kill = QStandardPaths::findExecutable("kill");
    if (kill.isEmpty()) return;
    const QStringList procs = QDir(QStringLiteral("/proc")).entryList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &entry : procs) {
        QFile cmdline(QStringLiteral("/proc/") + entry + QStringLiteral("/cmdline"));
        if (!cmdline.open(QIODevice::ReadOnly)) continue;
        bool isMpvpaper = false, underWallpaperDir = false;
        for (const QByteArray &a : cmdline.readAll().split('\0')) {
            if (a.isEmpty()) continue;
            const QString arg = QString::fromLocal8Bit(a);
            if (QFileInfo(arg).fileName() == QLatin1String("mpvpaper")) isMpvpaper = true;
            else if (arg.startsWith(prefix)) underWallpaperDir = true;
        }
        if (isMpvpaper && underWallpaperDir)
            QProcess::startDetached(kill, {QStringLiteral("-TERM"), entry});
    }
}

QString runAndCapture(const QString &bin, const QStringList &args, int timeoutMs = 2000) {
    QProcess p;
    p.start(bin, args);
    if (!p.waitForStarted(timeoutMs)) return {};
    if (!p.waitForFinished(timeoutMs)) { p.kill(); return {}; }
    return QString::fromUtf8(p.readAllStandardOutput());
}

// runs after any successful apply, image or video
ApplyResult finish(const QString &path, const ApplyResult &r,
                   const QString &stableCopyPath, const QString &postApplyCommand) {    // publish the wallpaper at a fixed path for lockscreens, bars, themes
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
                           const QString &postApplyCommand,
                           const QString &wallpaperDir) {
    ApplyResult r;
    r.backend = backendName(backend);
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        r.error = QStringLiteral("wallpaper not found");
        return r;
    }

    // A running video wallpaper must be stopped by any later apply, not just
    // by another video. mpvpaper loops forever and its layer outlives the
    // change awww makes underneath, so applying a still image left the video
    // on screen with no way back off it. The kill used to sit inside the
    // video branch, so only video-over-video was covered.
    stopVideoWallpaper(wallpaperDir);

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
        // No -f. With it mpvpaper forks, so the pid startDetached returns
        // belongs to a parent that exits at once and the real player is
        // reparented to init: players then pile up, each software-decoding
        // 4K AV1 at several hundred percent CPU.
        r.ok = run(mpvpaper, {"-o", "no-audio loop", monitor, path}, &r.error);
        if (!r.ok) return r;
        return finish(path, r, stableCopyPath, postApplyCommand);
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
        // swaybg stays in the foreground by design; startDetached is what
        // keeps it alive without blocking us, and we need its pid to replace
        // it on the next change
        r.ok = run(bin, {"-i", path, "-m", "fill"}, &r.error, &s_swaybgPid);
        break;
    }
    }

    if (!r.ok) return r;

    return finish(path, r, stableCopyPath, postApplyCommand);
}
