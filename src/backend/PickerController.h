#pragma once
#include <QObject>
#include "WallpaperRepository.h"
#include "WallpaperModel.h"
#include "WallpaperFilterProxy.h"
#include "AppConfig.h"

class PickerController : public QObject {
    Q_OBJECT
public:
    explicit PickerController(WallpaperRepository *repo, WallpaperModel *model, WallpaperFilterProxy *proxy, AppConfig *cfg, QObject *parent=nullptr);
    Q_INVOKABLE void setSearch(const QString &query);
    Q_INVOKABLE void applyWallpaper(const QString &path);
    void refresh();
signals:
    void wallpapersChanged();
    void wallpaperApplied(const QString &path, bool ok);
private:
    void applyFilters();
    WallpaperRepository *repo;
    WallpaperModel *model;
    WallpaperFilterProxy *proxy;
    AppConfig *cfg;
    QString searchQuery;
};
