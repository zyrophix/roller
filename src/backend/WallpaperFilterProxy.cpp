#include "WallpaperFilterProxy.h"

WallpaperFilterProxy::WallpaperFilterProxy(QObject *p): QSortFilterProxyModel(p) {
    connect(this, &QSortFilterProxyModel::rowsInserted, this, &WallpaperFilterProxy::countChanged);
    connect(this, &QSortFilterProxyModel::rowsRemoved, this, &WallpaperFilterProxy::countChanged);
    connect(this, &QSortFilterProxyModel::modelReset, this, &WallpaperFilterProxy::countChanged);
    connect(this, &QSortFilterProxyModel::layoutChanged, this, &WallpaperFilterProxy::countChanged);
    connect(this, &QSortFilterProxyModel::dataChanged, this, [this]{ bumpRev(); });
    connect(this, &QSortFilterProxyModel::modelReset, this, [this]{ bumpRev(); });
    connect(this, &QSortFilterProxyModel::layoutChanged, this, [this]{ bumpRev(); });
    // invalidateRowsFilter emits only rowsRemoved/rowsInserted, so without
    // these a refilter leaves rev untouched and get_*_at bindings keep serving
    // the pre-filter wallpaper for every unchanged slot
    connect(this, &QSortFilterProxyModel::rowsInserted, this, [this]{ bumpRev(); });
    connect(this, &QSortFilterProxyModel::rowsRemoved, this, [this]{ bumpRev(); });
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
    // The invalidate* family is deprecated from Qt 6.11, but endFilterChange
    // only arrives in 6.11 too: 6.9 has beginFilterChange alone and 6.8, which
    // is what CI builds, has neither and no deprecation either. Pick per
    // version so the build is warning-free on both ends of the range.
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
    beginFilterChange();
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateRowsFilter();
#endif
}
int WallpaperFilterProxy::count() const { return rowCount(); }
bool WallpaperFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &parent) const {
    if (!srcModel) return true;
    if (query.isEmpty()) return true;
    QModelIndex idx = srcModel->index(source_row, 0, parent);
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
