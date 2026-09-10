#include "WallpaperModel.h"
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QCryptographicHash>

WallpaperModel::WallpaperModel(QObject *p): QAbstractListModel(p) {}
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
    for (int i=0;i<m_items.size();++i) if (m_items[i].path==src) {
        m_items[i].thumb = QUrl::fromLocalFile(thumb).toString();
        emit dataChanged(index(i,0), index(i,0), {ThumbRole});
        break;
    }
}
void WallpaperModel::setItems(const QStringList &paths){
    beginResetModel();
    m_items.clear();
    for(auto &p: paths){
        QString name=QFileInfo(p).fileName();
        QString hash = QString::fromUtf8(QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Md5).toHex());
        QString ext = QFileInfo(p).suffix().toLower();
        if (ext.isEmpty()) ext = "jpg";
        QString thumbHashed = QDir(cacheDir).filePath(hash + "." + ext);
        // fall back to the original until ThumbnailCache produces the thumb
        QString thumb = QFileInfo::exists(thumbHashed) ? thumbHashed : p;
        m_items.append({p,name,QUrl::fromLocalFile(thumb).toString()});
    }
    endResetModel();
    emit countChanged();
}
QString WallpaperModel::get_path_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].path:""; }
QString WallpaperModel::get_name_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].name:""; }
QString WallpaperModel::get_thumb_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].thumb:""; }
