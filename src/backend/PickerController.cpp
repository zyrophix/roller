#include "PickerController.h"
#include "WallpaperBackend.h"

PickerController::PickerController(WallpaperRepository *r, WallpaperModel *m, WallpaperFilterProxy *px, AppConfig *c, QObject *p): QObject(p), repo(r), model(m), proxy(px), cfg(c) {}

void PickerController::refresh(){
    if (repo) repo->refresh();
    applyFilters();
    emit libraryRescanned();
}
void PickerController::applyFilters(){
    if (model && repo) model->setItems(repo->getAll());
    if (proxy) proxy->setSearchQuery(searchQuery);
    emit wallpapersChanged();
}
void PickerController::setSearch(const QString &q){
    QString nq = q.trimmed().toLower();
    if (nq == searchQuery) return;
    searchQuery = nq;
    if (proxy) proxy->setSearchQuery(q);
    emit wallpapersChanged();
}
void PickerController::applyWallpaper(const QString &path){
    ApplyResult r;
    if (model && cfg) {
        r = ::applyWallpaper(path,
                              ::resolveBackend(cfg->backend()),
                              cfg->transitionType(), cfg->transitionPos(),
                              cfg->transitionDuration(), cfg->transitionFps(),
                              cfg->videoExtensions(), cfg->stableCopyPath(),
                              cfg->postApplyCommand(), repo ? repo->path() : QString());
    } else {
        r.error = QStringLiteral("backend not initialised");
    }
    emit wallpaperApplied(path, r.ok, r.error);
}
