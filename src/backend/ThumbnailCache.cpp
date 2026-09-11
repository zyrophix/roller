#include "ThumbnailCache.h"
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QImageReader>
#include <QImage>
#include <QtConcurrent>
#include <QThreadPool>

ThumbnailCache::ThumbnailCache(const QString &dir, QObject *p): QObject(p), cacheDir(dir) {}

static int thumbPixelHeight(const QString &path) {
    QImageReader probe(path);
    QSize s = probe.size();
    return s.isValid() ? s.height() : -1;
}

void ThumbnailCache::generateMissing(const QStringList &paths) {
    QtConcurrent::run([this, paths]{
        QDir().mkpath(cacheDir);
        const int targetH = qMax(1, thumbHeight);
        for (auto &p : paths) {
            QString hash = QString::fromUtf8(QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Md5).toHex());
            QString ext = QFileInfo(p).suffix().toLower();
            if (ext.isEmpty()) ext = "jpg";
            QString thumb = QDir(cacheDir).filePath(hash + "." + ext);
            // skip only if the cached thumb is tall enough; older revisions
            // stored 500px-wide thumbs that upscale ~2x on a selected tile
            if (QFileInfo::exists(thumb) && thumbPixelHeight(thumb) >= targetH) continue;
            // migrate legacy basename-keyed thumbs produced by older versions
            QString legacy = QDir(cacheDir).filePath(QFileInfo(p).fileName());
            if (QFileInfo::exists(legacy) && thumbPixelHeight(legacy) >= targetH
                    && QFile::copy(legacy, thumb)) {
                QMetaObject::invokeMethod(this, [this, p, thumb]{ emit thumbReady(p, thumb); }, Qt::QueuedConnection);
                continue;
            }
            QImageReader r(p);
            r.setAutoTransform(true);
            if (!r.canRead()) continue;
            QSize src = r.size();
            // height-driven sizing, like `-thumbnail xH`: width follows aspect
            int h = targetH;
            int w = src.width() > 0 ? qMax(1, h * src.width() / qMax(1, src.height())) : h * 16 / 9;
            if (src.height() > 0 && src.height() < h) {
                w = src.width();
                h = src.height();
            }
            r.setScaledSize(QSize(w, h));
            QImage img = r.read();
            if (img.isNull()) continue;
            img.save(thumb, nullptr, 85);
            QMetaObject::invokeMethod(this, [this, p, thumb]{ emit thumbReady(p, thumb); }, Qt::QueuedConnection);
        }
    });
}
