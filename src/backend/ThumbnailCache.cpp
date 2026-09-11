#include "ThumbnailCache.h"
#include <QFileInfo>
#include <QDir>
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
    // snapshot into locals: the worker must not touch members off-thread
    const QString dir = cacheDir;
    const int targetH = qMax(1, thumbHeight);
    QPointer<ThumbnailCache> guard(this);
    mFuture = QtConcurrent::run([guard, dir, targetH, paths](QPromise<void> &promise){
        QDir().mkpath(dir);
        for (auto &p : paths) {
            if (promise.isCanceled()) return;
            QString hash = QString::fromUtf8(QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Md5).toHex());
            QString ext = QFileInfo(p).suffix().toLower();
            if (ext.isEmpty()) ext = "jpg";
            QString thumb = QDir(dir).filePath(hash + "." + ext);
            // skip only if the cached thumb is tall enough; older revisions
            // stored 500px-wide thumbs that upscale ~2x on a selected tile
            if (QFileInfo::exists(thumb) && thumbPixelHeight(thumb) >= targetH) continue;
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
    });
}
