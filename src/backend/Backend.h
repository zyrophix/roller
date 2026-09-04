#pragma once
#include <QObject>
#include "Repository.h"
#include "Metadata.h"
#include "Model.h"
#include "Config.h"

class Backend : public QObject {
    Q_OBJECT
public:
    explicit Backend(Repository *repo, MetadataStore *store, WallpaperModel *model, Config *cfg, QObject *parent=nullptr);
    Q_INVOKABLE void setFilter(const QString &color);
    Q_INVOKABLE void setSearch(const QString &query);
    Q_INVOKABLE void applyWallpaper(const QString &path);
    void refresh();
signals:
    void availableColorsChanged(const QStringList &colors);
    void activeColorChanged(const QString &color);
    void wallpapersChanged();
    void wallpaperApplied(const QString &path);
private:
    void applyFilters();
    Repository *repo;
    MetadataStore *store;
    WallpaperModel *model;
    Config *cfg;
    QString activeColor;
    QString searchQuery;
    QStringList available;
};
