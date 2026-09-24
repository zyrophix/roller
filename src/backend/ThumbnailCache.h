#pragma once
#include <QFuture>
#include <QObject>
#include <QStringList>

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
