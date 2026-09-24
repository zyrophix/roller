#include "WallpaperModel.h"
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QCryptographicHash>

WallpaperModel::WallpaperModel(QObject *p): QAbstractListModel(p) {}

// One shared naming rule for the cache, so the model and the generator
// cannot disagree. Always JPEG: png/bmp thumbs were ~2 MB each and gif/avif
// have no Qt writer, which left those wallpapers stuck on the full-size
// original forever.
QString thumbFileName(const QString &sourcePath) {
    const QByteArray h = QCryptographicHash::hash(sourcePath.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromUtf8(h) + QStringLiteral(".jpg");
}

// version the URL so an overwritten thumb (same path, new pixels) is
// treated as a new source by QML instead of served from the decode cache
static QString thumbUrl(const QString &thumb) {
    QString u = QUrl::fromLocalFile(thumb).toString();
    QFileInfo fi(thumb);
    if (fi.exists())
        u += QStringLiteral("?m=%1s%2").arg(fi.lastModified().toMSecsSinceEpoch()).arg(fi.size());
    return u;
}
int WallpaperModel::rowCount(const QModelIndex &p) const { return p.isValid()?0:m_items.size(); }
QVariant WallpaperModel::data(const QModelIndex &idx, int role) const {
    if (!idx.isValid() || idx.row()>=m_items.size()) return {};
    auto &it=m_items[idx.row()];
    if (role==PathRole) return it.path;
    if (role==NameRole) return it.name;
    if (role==ThumbRole) return it.thumb;
    return {};
}
QHash<int,QByteArray> WallpaperModel::roleNames() const {
    return {{PathRole,"wallpaperPath"},{NameRole,"wallpaperName"},{ThumbRole,"thumbnailPath"}};
}
void WallpaperModel::setDirs(const QString &w, const QString &c){ wallpaperDir=w; cacheDir=c; }
void WallpaperModel::onThumbReady(const QString &src, const QString &thumb) {
    auto it = m_rowByPath.constFind(src);
    if (it == m_rowByPath.constEnd()) return;
    int i = *it;
    m_items[i].thumb = thumbUrl(thumb);
    emit dataChanged(index(i,0), index(i,0), {ThumbRole});
}
void WallpaperModel::setItems(const QStringList &paths){
    beginResetModel();
    m_items.clear();
    m_rowByPath.clear();
    m_rowByPath.reserve(paths.size());
    for(auto &p: paths){
        QString name=QFileInfo(p).fileName();
        QString thumbHashed = QDir(cacheDir).filePath(thumbFileName(p));
        // fall back to the original until ThumbnailCache produces the thumb
        QString thumb = QFileInfo::exists(thumbHashed) ? thumbUrl(thumbHashed)
                                                          : QUrl::fromLocalFile(p).toString();
        m_rowByPath.insert(p, m_items.size());
        m_items.append({p,name,thumb});
    }
    endResetModel();
    emit countChanged();
}
QString WallpaperModel::get_path_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].path:""; }
QString WallpaperModel::get_name_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].name:""; }
QString WallpaperModel::get_thumb_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].thumb:""; }
