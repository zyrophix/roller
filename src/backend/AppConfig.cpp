#include "AppConfig.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

AppConfig::AppConfig(const QJsonObject &data, QObject *parent) : QObject(parent), obj(data) {}

QString AppConfig::lastWallpaper() const {
    if (stateFile.isEmpty()) return {};
    QFile f(stateFile);
    if (!f.open(QIODevice::ReadOnly)) return {};
    const QString p = QString::fromUtf8(f.readAll()).trimmed();
    // a stale entry (wallpaper deleted or moved) must not steer the carousel
    return (!p.isEmpty() && QFileInfo::exists(p)) ? p : QString{};
}

void AppConfig::saveLastWallpaper(const QString &path) {
    if (stateFile.isEmpty() || path.isEmpty()) return;
    QDir().mkpath(QFileInfo(stateFile).absolutePath());
    // QSaveFile: a torn write here would point the next launch at nothing
    QSaveFile f(stateFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(path.toUtf8());
    if (!f.commit()) return;
    emit changed();
}
