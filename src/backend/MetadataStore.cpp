#include "MetadataStore.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QSaveFile>
#include <QDebug>

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
