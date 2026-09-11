#pragma once
#include <QSortFilterProxyModel>
#include "WallpaperModel.h"

class WallpaperFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int countProp READ count NOTIFY countChanged)
    // data generation: QML bindings calling get_*_at are opaque function
    // calls that never re-evaluate on dataChanged. Reading rev in the same
    // binding subscribes it, so new thumbs and refilters propagate.
    Q_PROPERTY(int rev READ rev NOTIFY revChanged)
public:
    explicit WallpaperFilterProxy(QObject *parent=nullptr);
    int rev() const { return mRev; }
    void setSource(WallpaperModel *src);
    void setSearchQuery(const QString &q);
    Q_INVOKABLE int count() const;
    int countProp() const { return count(); }
    Q_INVOKABLE QString get_path_at(int idx) const;
    Q_INVOKABLE QString get_name_at(int idx) const;
    Q_INVOKABLE QString get_thumb_at(int idx) const;
    QHash<int, QByteArray> roleNames() const override;
signals:
    void countChanged();
    void revChanged();
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &parent) const override;
private:
    void bumpRev() { ++mRev; emit revChanged(); }
    QString query;
    WallpaperModel *srcModel = nullptr;
    int mRev = 0;
};
