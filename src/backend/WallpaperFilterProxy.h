#pragma once
#include <QSortFilterProxyModel>
#include "WallpaperModel.h"

class WallpaperFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int countProp READ count NOTIFY countChanged)
public:
    explicit WallpaperFilterProxy(QObject *parent=nullptr);
    void setSource(WallpaperModel *src);
    void setSearchQuery(const QString &q);
    void setColorGroup(const QString &g);
    Q_INVOKABLE int count() const;
    int countProp() const { return count(); }
    Q_INVOKABLE QString get_path_at(int idx) const;
    Q_INVOKABLE QString get_name_at(int idx) const;
    Q_INVOKABLE QString get_thumb_at(int idx) const;
    QHash<int, QByteArray> roleNames() const override;
signals:
    void countChanged();
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &parent) const override;
private:
    QString query;
    QString colorGroup;
    WallpaperModel *srcModel = nullptr;
};
