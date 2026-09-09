#pragma once
#include <QString>
#include <QMap>
#include <QVariant>

class MetadataStore {
public:
    explicit MetadataStore(const QString &path);
    QMap<QString, QVariantMap> load();
    void save();
    QVariantMap get(const QString &filename) const;
    void set(const QString &filename, double mtime, const QString &dominant, const QString &group);
    bool isCurrent(const QString &filename, double mtime) const;
    bool updateFromThumbnail(const QString &wallpaperPath, const QString &thumbPath);
    void removeMissing(const QStringList &filenames);
    QMap<QString, QVariantMap> data;

private:
    QString filePath;
    static QMap<QString, QVariantMap> loadFile(const QString &p);
};
