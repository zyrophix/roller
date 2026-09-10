#include "WallpaperRepository.h"
#include <QDirIterator>

const QStringList WallpaperRepository::kExts = {"*.jpg","*.jpeg","*.png","*.webp","*.bmp"};

WallpaperRepository::WallpaperRepository(const QString &d): dir(d) {}

void WallpaperRepository::refresh() {
    wallpapers.clear();
    QDirIterator it(dir, kExts, QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        wallpapers << it.filePath();
    }
    std::sort(wallpapers.begin(), wallpapers.end(), [](const QString &a, const QString &b){
        return QFileInfo(a).fileName().toLower() < QFileInfo(b).fileName().toLower();
    });
}
QStringList WallpaperRepository::getAll() const { return wallpapers; }
