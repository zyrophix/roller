#pragma once
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>

class WallpaperRepository {
public:
    explicit WallpaperRepository(const QString &wallpaperDir);
    void refresh();
    QStringList getAll() const; // absolute paths

private:
    QString dir;
    QStringList wallpapers; // sorted absolute paths
    static const QStringList kExts;
};
