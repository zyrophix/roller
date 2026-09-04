#include "Backend.h"
#include "Awww.h"

Backend::Backend(Repository *r, MetadataStore *s, WallpaperModel *m, Config *c, QObject *p): QObject(p), repo(r), store(s), model(m), cfg(c) {}

void Backend::refresh(){
    store->load();
    repo->setMetadata(store->data);
    available = repo->availableColors();
    emit availableColorsChanged(available);
    emit activeColorChanged(activeColor);
    applyFilters();
}
void Backend::applyFilters(){
    QStringList all = repo->getAll();
    QStringList filtered = all;
    if (!activeColor.isEmpty()) filtered = repo->filterByColor(activeColor);
    if (!searchQuery.trimmed().isEmpty()) {
        // filter already color-filtered, now name
        QString q = searchQuery.trimmed().toLower();
        QStringList tmp;
        for(auto &p: filtered) if (QFileInfo(p).fileName().toLower().contains(q)) tmp<<p;
        filtered = tmp;
    }
    model->setItems(filtered, store->data);
    emit wallpapersChanged();
}
void Backend::setFilter(const QString &c){ activeColor=c; emit activeColorChanged(c); applyFilters(); }
void Backend::setSearch(const QString &q){ searchQuery=q; applyFilters(); }
void Backend::applyWallpaper(const QString &path){
    ::applyWallpaper(path, cfg->transitionType(), cfg->transitionPos(), cfg->transitionDuration(), cfg->transitionFps());
    emit wallpaperApplied(path);
}
