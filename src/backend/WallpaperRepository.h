#pragma once
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QVariant>

class WallpaperRepository {
public:
    explicit WallpaperRepository(const QString &wallpaperDir);
    void refresh();
    void setMetadata(const QMap<QString, QVariantMap> &meta);
    QStringList getAll() const; // absolute paths
    QStringList filterByColor(const QString &group) const;
    QStringList filterByName(const QString &query) const;
    QStringList availableColors() const;

private:
    QString dir;
    QStringList wallpapers; // sorted absolute paths
    QMap<QString, QVariantMap> metadata; // filename -> {color_group...}
    static const QStringList kExts;
};
