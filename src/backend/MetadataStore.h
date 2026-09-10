#pragma once
#include <QString>
#include <QMap>
#include <QVariant>

class MetadataStore {
public:
    explicit MetadataStore(const QString &path);
    QMap<QString, QVariantMap> load();
    void save();
    QMap<QString, QVariantMap> data;

private:
    QString filePath;
    static QMap<QString, QVariantMap> loadFile(const QString &p);
};
