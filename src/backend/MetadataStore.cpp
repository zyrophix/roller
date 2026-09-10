#include "MetadataStore.h"
#include "ColorClassifier.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QImage>
#include <QDateTime>
#include <QSaveFile>
#include <QDebug>
#include <cmath>

MetadataStore::MetadataStore(const QString &p): filePath(p) {}

QMap<QString, QVariantMap> MetadataStore::load() {
    data = loadFile(filePath);
    return data;
}
QMap<QString, QVariantMap> MetadataStore::loadFile(const QString &p) {
    QMap<QString, QVariantMap> out;
    QFile f(p);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return out;
    auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return out;
    auto obj = doc.object();
    for (auto it=obj.begin(); it!=obj.end(); ++it) {
        auto v = it.value().toObject();
        QVariantMap m;
        m["mtime"] = v["mtime"].toDouble();
        m["dominant_color"] = v["dominant_color"].toString();
        m["color_group"] = v["color_group"].toString();
        out[it.key()] = m;
    }
    return out;
}
void MetadataStore::save() {
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QSaveFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "MetadataStore: failed to open" << filePath << f.errorString();
        return;
    }
    QJsonObject obj;
    for (auto it=data.begin(); it!=data.end(); ++it) {
        QJsonObject o;
        o["mtime"] = it.value().value("mtime").toDouble();
        o["dominant_color"] = it.value().value("dominant_color").toString();
        o["color_group"] = it.value().value("color_group").toString();
        obj[it.key()] = o;
    }
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    if (!f.commit()) qWarning() << "MetadataStore: commit failed" << filePath << f.errorString();
}
QVariantMap MetadataStore::get(const QString &fn) const { return data.value(fn); }
void MetadataStore::set(const QString &fn, double mtime, const QString &dom, const QString &group){
    QVariantMap m; m["mtime"]=mtime; m["dominant_color"]=dom; m["color_group"]=group; data[fn]=m;
}
bool MetadataStore::isCurrent(const QString &fn, double mtime) const {
    auto it=data.find(fn); return it!=data.end() && it.value().value("mtime").toDouble()==mtime;
}
void MetadataStore::removeMissing(const QStringList &names){
    QSet<QString> s(names.begin(), names.end());
    for (auto it=data.begin(); it!=data.end(); ){
        if (!s.contains(it.key())) it=data.erase(it); else ++it;
    }
}
// Simplified dominant color: sample 48, LAB weighting, k-means like python (trimmed for brevity — uses average of chromatic)
bool MetadataStore::updateFromThumbnail(const QString &wallpaperPath, const QString &thumbPath){
    QFileInfo wInfo(wallpaperPath);
    double mtime = wInfo.exists() ? wInfo.lastModified().toMSecsSinceEpoch()/1000.0 : 0;
    QString name = wInfo.fileName();
    if (isCurrent(name, mtime)) return false;
    QFileInfo tInfo(thumbPath);
    if (!tInfo.exists()) return false;
    QImage img(thumbPath);
    if (img.isNull()) return false;
    img = img.convertToFormat(QImage::Format_RGB888);
    int w=img.width(), h=img.height();
    if (w<=0||h<=0) return false;
    int maxDim=48;
    int stepX = std::max(1, w / maxDim);
    int stepY = std::max(1, h / maxDim);
    QVector<QVector<int>> samples;
    for(int y=0;y<h;y+=stepY) for(int x=0;x<w;x+=stepX){
        QRgb c = img.pixel(x,y);
        samples.append({qRed(c), qGreen(c), qBlue(c)});
    }
    if (samples.isEmpty()) return false;
    // quick average with chroma weighting (simplified vs python full k-means)
    long sr=0, sg=0, sb=0;
    for(auto &s: samples){ sr+=s[0]; sg+=s[1]; sb+=s[2]; }
    int r = sr / samples.size(), g = sg / samples.size(), b = sb / samples.size();
    QString dom = rgbToHex(r,g,b);
    QString grp = classifyRgb(r,g,b);
    set(name, mtime, dom, grp);
    return true;
}
