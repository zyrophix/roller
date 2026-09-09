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
void WallpaperRepository::setMetadata(const QMap<QString, QVariantMap> &m){ metadata=m; }
QStringList WallpaperRepository::getAll() const { return wallpapers; }
QStringList WallpaperRepository::filterByColor(const QString &group) const {
    if (group.isEmpty()) return wallpapers;
    QStringList out;
    for (auto &p: wallpapers) {
        QString name = QFileInfo(p).fileName();
        auto it = metadata.find(name);
        if (it!=metadata.end() && it.value().value("color_group").toString()==group) out<<p;
    }
    return out;
}
QStringList WallpaperRepository::filterByName(const QString &query) const {
    QString q = query.trimmed().toLower();
    if (q.isEmpty()) return wallpapers;
    QStringList out;
    for (auto &p: wallpapers) if (QFileInfo(p).fileName().toLower().contains(q)) out<<p;
    return out;
}
QStringList WallpaperRepository::availableColors() const {
    QStringList colors; QSet<QString> seen;
    for (auto &p: wallpapers) {
        QString name=QFileInfo(p).fileName();
        auto it=metadata.find(name);
        if (it==metadata.end()) continue;
        QString g=it.value().value("color_group").toString();
        if (g.isEmpty() || seen.contains(g)) continue;
        seen.insert(g); colors<<g;
    }
    return colors;
}
