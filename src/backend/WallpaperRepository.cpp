#include "WallpaperRepository.h"
#include <QDirIterator>
#include <QSet>

const QStringList WallpaperRepository::kExts = {"*.jpg","*.jpeg","*.png","*.webp","*.bmp"};

namespace {
const QSet<QString> kOkSuffix = {"jpg","jpeg","png","webp","bmp","gif","avif"};
}

WallpaperRepository::WallpaperRepository(const QString &d): dir(d) {}

void WallpaperRepository::refresh() {
    wallpapers.clear();
    // match by lowercased suffix instead of glob filters: QDir name filters
    // are case-sensitive, so IMG.JPG never matched. Symlinked files are
    // listed as-is; symlinked dirs are not descended into (no cycle risk).
    QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (kOkSuffix.contains(QFileInfo(it.filePath()).suffix().toLower()))
            wallpapers << it.filePath();
    }
    std::sort(wallpapers.begin(), wallpapers.end(), [](const QString &a, const QString &b){
        return QFileInfo(a).fileName().toLower() < QFileInfo(b).fileName().toLower();
    });
}
QStringList WallpaperRepository::getAll() const { return wallpapers; }
