#include "PickerController.h"
#include "Awww.h"

PickerController::PickerController(WallpaperRepository *r, WallpaperModel *m, WallpaperFilterProxy *px, AppConfig *c, QObject *p): QObject(p), repo(r), model(m), proxy(px), cfg(c) {}

void PickerController::refresh(){
    if (repo) repo->refresh();
    applyFilters();
}
void PickerController::applyFilters(){
    model->setItems(repo->getAll());
    if (proxy) proxy->setSearchQuery(searchQuery);
    emit wallpapersChanged();
}
void PickerController::setSearch(const QString &q){
    if (q == searchQuery) return;
    searchQuery = q;
    if (proxy) proxy->setSearchQuery(q);
    emit wallpapersChanged();
}
void PickerController::applyWallpaper(const QString &path){
    ::applyWallpaper(path, cfg->transitionType(), cfg->transitionPos(), cfg->transitionDuration(), cfg->transitionFps());
    emit wallpaperApplied(path);
}
