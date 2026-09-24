#include "ThumbnailCache.h"
#include "WallpaperModel.h"
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QCryptographicHash>
#include <QFile>
#include <QImageReader>
#include <QImage>
#include <QPointer>
#include <QPromise>
#include <QtConcurrent>
#include <QThreadPool>

ThumbnailCache::ThumbnailCache(const QString &dir, QObject *p): QObject(p), cacheDir(dir) {}

ThumbnailCache::~ThumbnailCache() {
    mFuture.cancel();
    mFuture.waitForFinished();
}

static int thumbPixelHeight(const QString &path) {
    QImageReader probe(path);
    probe.setAutoTransform(true);
    QSize s = probe.size();
    return s.isValid() ? s.height() : -1;
}

static void emitThumbReady(QPointer<ThumbnailCache> guard, const QString &p, const QString &thumb) {
    if (!guard) return;
    QMetaObject::invokeMethod(guard, [guard, p, thumb]{
        if (guard) emit guard->thumbReady(p, thumb);
    }, Qt::QueuedConnection);
}

void ThumbnailCache::generateMissing(const QStringList &paths) {
    // a previous pass may still be writing the same files
    mFuture.cancel();
    mFuture.waitForFinished();
    // snapshot into locals: the worker must not touch members off-thread
    const QString dir = cacheDir;
    const int targetH = qMax(1, thumbHeight);
    const qint64 maxBytes = this->maxBytes;
    QPointer<ThumbnailCache> guard(this);
    mFuture = QtConcurrent::run([guard, dir, targetH, maxBytes, paths](QPromise<void> &promise){
        QDir().mkpath(dir);
        QSet<QString> live;
        for (auto &p : paths) {
            if (promise.isCanceled()) return;
            const QString thumb = QDir(dir).filePath(thumbFileName(p));
            live.insert(QFileInfo(thumb).fileName());
            // "good enough" means close to the target, not merely larger:
            // a stale 550px thumb is still over the 2 MiB per-entry pixmap
            // cache limit once the target drops to 512.
            const int cachedH = QFileInfo::exists(thumb) ? thumbPixelHeight(thumb) : -1;
            const bool ok = cachedH >= targetH && cachedH <= targetH + targetH / 16;
            if (ok) continue;
            // migrate legacy basename-keyed thumbs produced by older versions.
            // the aspect check guards against two sources sharing a basename
            // in different subdirs: a foreign legacy thumb never matches.
            QString legacy = QDir(dir).filePath(QFileInfo(p).fileName());
            if (QFileInfo::exists(legacy) && thumbPixelHeight(legacy) >= targetH) {
                QImageReader lp(legacy), sp(p);
                lp.setAutoTransform(true);
                sp.setAutoTransform(true);
                QSize ls = lp.size(), ss = sp.size();
                qint64 left = qint64(ls.width()) * ss.height();
                qint64 right = qint64(ss.width()) * ls.height();
                if (ls.isValid() && ss.isValid() && right > 0
                        && qAbs(left - right) * 50 <= right
                        && QFile::copy(legacy, thumb)) {
                    emitThumbReady(guard, p, thumb);
                    continue;
                }
            }
            QImageReader r(p);
            r.setAutoTransform(true);
            if (!r.canRead()) continue;
            QSize src = r.size();
            if (!src.isValid() || src.width() <= 0 || src.height() <= 0) continue;
            // height-driven sizing, like `-thumbnail xH`: width follows aspect
            int h = targetH;
            int w = h * src.width() / src.height();
            if (src.height() < h) {
                w = src.width();
                h = src.height();
            }
            // clamp against corrupt headers claiming absurd aspects
            w = qBound(1, w, qMin(4096, 4 * h));
            h = qMin(h, 4096);
            r.setScaledSize(QSize(w, h));
            QImage img = r.read();
            if (img.isNull()) continue;
            if (!img.save(thumb, nullptr, 85)) {
                QFile::remove(thumb);
                continue;
            }
            emitThumbReady(guard, p, thumb);
        }
        // drop thumbs whose wallpaper is gone, then enforce the size bound
        // oldest-first. legacy basename-keyed files are left alone: the
        // migration above still consumes them.
        QDir d(dir);
        struct Entry { QString name; qint64 bytes; QDateTime mtime; };
        QVector<Entry> entries;
        qint64 total = 0;
        for (const auto &name : d.entryList(QDir::Files, QDir::Name)) {
            if (QFileInfo(name).completeBaseName().size() != 32) continue;
            const QFileInfo fi(d.filePath(name));
            entries.append({name, fi.size(), fi.lastModified()});
            total += fi.size();
        }
        std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
            return a.mtime < b.mtime;
        });
        for (const auto &e : entries) {
            if (promise.isCanceled()) return;
            if (live.contains(e.name)) continue;
            if (QFile::remove(d.filePath(e.name))) total -= e.bytes;
        }
        for (const auto &e : entries) {
            if (promise.isCanceled()) return;
            if (total <= maxBytes) break;
            if (!live.contains(e.name)) continue;   // regenerate instead of thrash
            if (QFile::remove(d.filePath(e.name))) total -= e.bytes;
        }
    });
}
