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
        WallpaperRequest req;
        req.path = path;
        req.backend = ::resolveBackend(cfg->backend());
        req.transitionType = cfg->transitionType();
        req.transitionPos = cfg->transitionPos();
        req.transitionDuration = cfg->transitionDuration();
        req.transitionFps = cfg->transitionFps();
        req.videoExtensions = cfg->videoExtensions();
        req.videoHwdec = cfg->videoHwdec();
        req.stableCopyPath = cfg->stableCopyPath();
        req.postApplyCommand = cfg->postApplyCommand();
        req.wallpaperDir = repo ? repo->path() : QString();
        r = ::applyWallpaper(req);
    } else {
        r.error = QStringLiteral("backend not initialised");
    }
    emit wallpaperApplied(path, r.ok, r.error);
}
