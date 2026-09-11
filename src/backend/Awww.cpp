#include "Awww.h"
#include <QStandardPaths>
#include <QProcess>
#include <QFileInfo>
#include <QSet>
#include <cmath>

QString findAwwwBin(){
    for(auto &n: QStringList{"awww","swww"}){
        auto p=QStandardPaths::findExecutable(n);
        if(!p.isEmpty()) return p;
    }
    return {};
}
namespace {
const QSet<QString> kOkTransitions = {"none","simple","fade","left","right","top","bottom",
                                       "center","outer","any","grow","wipe","wave"};
}

bool applyWallpaper(const QString &path, const QString &type, const QString &pos, double duration, int fps){
    QString bin=findAwwwBin();
    if(bin.isEmpty() || !QFileInfo::exists(path)) return false;
    // never pass raw config text as flags: whitelist the transition type,
    // validate the position pair, clamp the numbers
    QString t = kOkTransitions.contains(type) ? type : QStringLiteral("grow");
    QStringList xy = pos.split(',');
    bool okX = false, okY = false;
    double x = xy.value(0).toDouble(&okX), y = xy.value(1).toDouble(&okY);
    QString p = (okX && okY) ? QStringLiteral("%1,%2").arg(x).arg(y) : QStringLiteral("0.5,0.5");
    double d = std::isfinite(duration) ? qBound(0.0, duration, 10.0) : 1.2;
    int f = fps > 0 ? qMin(fps, 240) : 60;
    QStringList args={"img", path, "--transition-type", t, "--transition-pos", p,
                      "--transition-duration", QString::number(d), "--transition-fps", QString::number(f)};
    return QProcess::startDetached(bin, args);
}
