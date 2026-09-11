#pragma once
#include <QObject>
#include <QStringList>

class ThumbnailCache : public QObject {
    Q_OBJECT
public:
    explicit ThumbnailCache(const QString &cacheDir, QObject *parent=nullptr);
    void setTargetHeight(int h) { thumbHeight = qMax(1, h); }
    void generateMissing(const QStringList &paths);
signals:
    void thumbReady(const QString &sourcePath, const QString &thumbPath);
private:
    QString cacheDir;
    int thumbHeight = 500;
};
