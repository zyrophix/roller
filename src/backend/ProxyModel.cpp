#include "ProxyModel.h"

WallpaperFilterProxy::WallpaperFilterProxy(QObject *p): QSortFilterProxyModel(p) {
    connect(this, &QSortFilterProxyModel::rowsInserted, this, &WallpaperFilterProxy::countChanged);
    connect(this, &QSortFilterProxyModel::rowsRemoved, this, &WallpaperFilterProxy::countChanged);
    connect(this, &QSortFilterProxyModel::modelReset, this, &WallpaperFilterProxy::countChanged);
    connect(this, &QSortFilterProxyModel::layoutChanged, this, &WallpaperFilterProxy::countChanged);
}
QHash<int, QByteArray> WallpaperFilterProxy::roleNames() const {
    if (srcModel) return srcModel->roleNames();
    return QSortFilterProxyModel::roleNames();
}
void WallpaperFilterProxy::setSource(WallpaperModel *src){
    srcModel = src;
    setSourceModel(src);
}
void WallpaperFilterProxy::setSearchQuery(const QString &q){
    QString nq = q.trimmed().toLower();
    if (nq == query) return;
    query = nq;
    invalidateRowsFilter();
}
void WallpaperFilterProxy::setColorGroup(const QString &g){
    if (g == colorGroup) return;
    colorGroup = g;
    invalidateRowsFilter();
}
int WallpaperFilterProxy::count() const { return rowCount(); }
bool WallpaperFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &parent) const {
    if (!srcModel) return true;
    QModelIndex idx = srcModel->index(source_row, 0, parent);
    if (!colorGroup.isEmpty()) {
        QString c = srcModel->data(idx, WallpaperModel::ColorRole).toString();
        if (c != colorGroup) return false;
    }
    if (query.isEmpty()) return true;
    QString name = srcModel->data(idx, WallpaperModel::NameRole).toString().toLower();
    return name.contains(query);
}
QString WallpaperFilterProxy::get_path_at(int idx) const {
    if (idx < 0 || idx >= rowCount()) return "";
    QModelIndex pIdx = index(idx, 0);
    QModelIndex sIdx = mapToSource(pIdx);
    return srcModel ? srcModel->data(sIdx, WallpaperModel::PathRole).toString() : "";
}
QString WallpaperFilterProxy::get_name_at(int idx) const {
    if (idx < 0 || idx >= rowCount()) return "";
    QModelIndex pIdx = index(idx, 0);
    QModelIndex sIdx = mapToSource(pIdx);
    return srcModel ? srcModel->data(sIdx, WallpaperModel::NameRole).toString() : "";
}
QString WallpaperFilterProxy::get_thumb_at(int idx) const {
    if (idx < 0 || idx >= rowCount()) return "";
    QModelIndex pIdx = index(idx, 0);
    QModelIndex sIdx = mapToSource(pIdx);
    return srcModel ? srcModel->data(sIdx, WallpaperModel::ThumbRole).toString() : "";
}
