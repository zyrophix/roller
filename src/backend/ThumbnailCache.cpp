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

void ThumbnailCache::generateMissing(const QStringList &paths) {
    QtConcurrent::run([this, paths]{
        QDir().mkpath(cacheDir);
        for (auto &p : paths) {
            QString hash = QString::fromUtf8(QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Md5).toHex());
            QString ext = QFileInfo(p).suffix().toLower();
            if (ext.isEmpty()) ext = "jpg";
            QString thumb = QDir(cacheDir).filePath(hash + "." + ext);
            if (QFileInfo::exists(thumb)) continue;
            // migrate legacy basename-keyed thumbs produced by older versions
            QString legacy = QDir(cacheDir).filePath(QFileInfo(p).fileName());
            if (QFileInfo::exists(legacy) && QFile::copy(legacy, thumb)) {
                QMetaObject::invokeMethod(this, [this, p, thumb]{ emit thumbReady(p, thumb); }, Qt::QueuedConnection);
                continue;
            }
            QImageReader r(p);
            r.setAutoTransform(true);
            if (!r.canRead()) continue;
            r.setScaledSize(QSize(500, 500 * r.size().height() / qMax(1, r.size().width())));
            QImage img = r.read();
            if (img.isNull()) continue;
            img.save(thumb, nullptr, 85);
            QMetaObject::invokeMethod(this, [this, p, thumb]{ emit thumbReady(p, thumb); }, Qt::QueuedConnection);
        }
    });
}
