#pragma once
#include <QString>
#include <QMap>

struct ColorGroup {
    QString name;
    QString hex;
};

extern const QMap<QString, ColorGroup> kColorGroups;

QString rgbToHex(int r, int g, int b);
QString classifyRgb(int r, int g, int b);
QString getGroupColor(const QString &name);
