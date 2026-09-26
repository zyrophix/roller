#pragma once
#include <QFuture>
#include <QObject>
#include <QSize>
#include <QStringList>

// Height-driven sizing, like `magick -thumbnail xH`: the target is the height
// and the width follows the source aspect. Sources smaller than the target
// are never upscaled, and a corrupt header claiming an absurd aspect is
// clamped. Kept out of the worker so it can be tested - sizing by width
// instead of height is what made every tile look soft.
QSize thumbnailSize(const QSize &source, int targetHeight);

class ThumbnailCache : public QObject {
    Q_OBJECT
public:
    explicit ThumbnailCache(const QString &cacheDir, QObject *parent=nullptr);
    ~ThumbnailCache() override;
    void setTargetHeight(int h) { thumbHeight = qMax(1, h); }
    void setMaxBytes(qint64 b) { maxBytes = qMax<qint64>(1, b); }
    void generateMissing(const QStringList &paths);
signals:
    void thumbReady(const QString &sourcePath, const QString &thumbPath);
private:
    QString cacheDir;
    int thumbHeight = 500;
    qint64 maxBytes = 256LL * 1024 * 1024;
    QFuture<void> mFuture;
};
