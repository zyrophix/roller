#include "WallpaperModel.h"
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QCryptographicHash>
#include <QImage>
#include <QImageReader>

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
        QString hash = QString::fromUtf8(QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Md5).toHex());
        QString ext = QFileInfo(p).suffix().toLower();
        if (ext.isEmpty()) ext = "jpg";
        QString hashedName = hash + "." + ext;
        QString thumbHashed = QDir(cacheDir).filePath(hashedName);
        QString thumbOld = QDir(cacheDir).filePath(name);
        QString thumb;
        if (QFileInfo::exists(thumbHashed)) thumb = thumbHashed;
        else if (QFileInfo::exists(thumbOld)) {
            // migrate old basename thumb to hashed name on first use
            QFile::copy(thumbOld, thumbHashed);
            thumb = thumbHashed;
        } else {
            // auto-generate thumb so roller needs no manual cache.sh
            QImageReader reader(p);
            reader.setAutoTransform(true);
            QImage img = reader.read();
            if (!img.isNull()) {
                QImage scaled = img.scaledToWidth(500, Qt::SmoothTransformation);
                QDir().mkpath(cacheDir);
                if (scaled.save(thumbHashed, nullptr, 85)) thumb = thumbHashed;
                else thumb = p;
            } else thumb = p;
        }
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
