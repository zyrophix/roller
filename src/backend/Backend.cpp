#include "Backend.h"
#include "Awww.h"

Backend::Backend(Repository *r, MetadataStore *s, WallpaperModel *m, WallpaperFilterProxy *px, Config *c, QObject *p): QObject(p), repo(r), store(s), model(m), proxy(px), cfg(c) {}

void Backend::refresh(){
    store->load();
    repo->setMetadata(store->data);
    available = repo->availableColors();
    emit availableColorsChanged(available);
    emit activeColorChanged(activeColor);
    applyFilters();
}
void Backend::applyFilters(){
    // Source model is built once (rescan path); color + text live in the proxy
    model->setItems(repo->getAll(), store->data);
    if (proxy) {
        proxy->setColorGroup(activeColor);
        proxy->setSearchQuery(searchQuery);
    }
    emit wallpapersChanged();
}
void Backend::setFilter(const QString &c){
    if (c == activeColor) return;
    activeColor = c;
    emit activeColorChanged(c);
    if (proxy) proxy->setColorGroup(c);
    emit wallpapersChanged();
}
void Backend::setSearch(const QString &q){
    if (q == searchQuery) return;
    searchQuery = q;
    if (proxy) proxy->setSearchQuery(q);
    emit wallpapersChanged();
}
void Backend::applyWallpaper(const QString &path){
    ::applyWallpaper(path, cfg->transitionType(), cfg->transitionPos(), cfg->transitionDuration(), cfg->transitionFps());
    emit wallpaperApplied(path);
}
