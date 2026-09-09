#pragma once
#include <QAbstractListModel>
#include <QStringList>
#include <QMap>
#include <QVariant>

class WallpaperModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { PathRole = Qt::UserRole+1, NameRole, ThumbRole, ColorRole };
    explicit WallpaperModel(QObject *parent=nullptr);
    int rowCount(const QModelIndex &p=QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    QHash<int,QByteArray> roleNames() const override;

    void setDirs(const QString &wallpaperDir, const QString &cacheDir);
    void setItems(const QStringList &paths, const QMap<QString, QVariantMap> &meta);

    Q_INVOKABLE int count() const { return m_items.size(); }
    Q_INVOKABLE QString get_path_at(int idx) const;
    Q_INVOKABLE QString get_name_at(int idx) const;
    Q_INVOKABLE QString get_thumb_at(int idx) const;
    Q_PROPERTY(int countProp READ count NOTIFY countChanged)
    int countProp() const { return m_items.size(); }
signals:
    void countChanged();
private:
    struct Item { QString path, name, thumb, color; };
    QVector<Item> m_items;
    QString wallpaperDir, cacheDir;
};
