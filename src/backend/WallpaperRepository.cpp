#include "WallpaperRepository.h"
#include <QDirIterator>
#include <QSet>

namespace {
const QSet<QString> kOkSuffix = {"jpg","jpeg","png","webp","bmp","gif","avif"};
}

WallpaperRepository::WallpaperRepository(const QString &d): dir(d) {}

void WallpaperRepository::refresh() {
    wallpapers.clear();
    QSet<QString> ok = kOkSuffix;
    for (const auto &e : videoExts) {
        const QString s = e.trimmed().toLower();
        if (!s.isEmpty()) ok.insert(s);
    }
    // match by lowercased suffix instead of glob filters: QDir name filters
    // are case-sensitive, so IMG.JPG never matched. Symlinked files are
    // listed as-is; symlinked dirs are not descended into (no cycle risk).
    QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (ok.contains(QFileInfo(it.filePath()).suffix().toLower()))
            wallpapers << it.filePath();
    }
    std::sort(wallpapers.begin(), wallpapers.end(), [](const QString &a, const QString &b){
        // name first so the order stays familiar, full path as tiebreaker:
        // two wallpapers with the same basename in different subdirs
        // otherwise compared equal and could swap places between launches
        const QString an = QFileInfo(a).fileName().toLower();
        const QString bn = QFileInfo(b).fileName().toLower();
        if (an != bn) return an < bn;
        return a.toLower() < b.toLower();
    });
}
QStringList WallpaperRepository::getAll() const { return wallpapers; }
