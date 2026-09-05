#include "Model.h"
#include <QFileInfo>
#include <QDir>
#include <QUrl>

WallpaperModel::WallpaperModel(QObject *p): QAbstractListModel(p) {}
int WallpaperModel::rowCount(const QModelIndex &p) const { return p.isValid()?0:m_items.size(); }
QVariant WallpaperModel::data(const QModelIndex &idx, int role) const {
    if (!idx.isValid() || idx.row()>=m_items.size()) return {};
    auto &it=m_items[idx.row()];
    if (role==PathRole) return it.path;
    if (role==NameRole) return it.name;
    if (role==ThumbRole) return it.thumb;
    if (role==ColorRole) return it.color;
    return {};
}
QHash<int,QByteArray> WallpaperModel::roleNames() const {
    return {{PathRole,"wallpaperPath"},{NameRole,"wallpaperName"},{ThumbRole,"thumbnailPath"},{ColorRole,"colorGroup"}};
}
void WallpaperModel::setDirs(const QString &w, const QString &c){ wallpaperDir=w; cacheDir=c; }
void WallpaperModel::setItems(const QStringList &paths, const QMap<QString, QVariantMap> &meta){
    beginResetModel();
    m_items.clear();
    for(auto &p: paths){
        QString name=QFileInfo(p).fileName();
        QString thumb = QDir(cacheDir).filePath(name);
        if (!QFileInfo::exists(thumb)) thumb=p;
        QString thumbUrl = QUrl::fromLocalFile(thumb).toString();
        QString color;
        auto it=meta.find(name);
        if (it!=meta.end()) color=it.value().value("color_group").toString();
        m_items.append({p,name,thumbUrl,color});
    }
    endResetModel();
    emit countChanged();
}
QString WallpaperModel::get_path_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].path:""; }
QString WallpaperModel::get_name_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].name:""; }
QString WallpaperModel::get_thumb_at(int idx) const { return (idx>=0&&idx<m_items.size())?m_items[idx].thumb:""; }
