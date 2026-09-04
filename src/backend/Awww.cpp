#include "Awww.h"
#include <QStandardPaths>
#include <QProcess>
#include <QFileInfo>

QString findAwwwBin(){
    for(auto &n: QStringList{"awww","swww"}){
        auto p=QStandardPaths::findExecutable(n);
        if(!p.isEmpty()) return p;
    }
    return {};
}
bool applyWallpaper(const QString &path, const QString &type, const QString &pos, double duration, int fps){
    QString bin=findAwwwBin();
    if(bin.isEmpty() || !QFileInfo::exists(path)) return false;
    QStringList args={"img", path, "--transition-type", type, "--transition-pos", pos, "--transition-duration", QString::number(duration), "--transition-fps", QString::number(fps)};
    QProcess::startDetached(bin, args);
    return true;
}
