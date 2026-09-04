#include "Color.h"
#include <cmath>

const QMap<QString, ColorGroup> kColorGroups = {
    {"red", {"red", "#E53935"}},
    {"orange", {"orange", "#FB8C00"}},
    {"yellow", {"yellow", "#FDD835"}},
    {"green", {"green", "#43A047"}},
    {"cyan", {"cyan", "#00ACC1"}},
    {"blue", {"blue", "#1E88E5"}},
    {"purple", {"purple", "#8E24AA"}},
    {"pink", {"pink", "#D81B60"}},
    {"gray", {"gray", "#9E9E9E"}},
};

QString rgbToHex(int r, int g, int b) {
    return QString("#%1%2%3").arg(r, 2, 16, QChar('0')).arg(g, 2, 16, QChar('0')).arg(b, 2, 16, QChar('0')).toUpper();
}

static double srgbToLinear(double v) {
    v /= 255.0;
    if (v <= 0.04045) return v / 12.92;
    return std::pow((v + 0.055) / 1.055, 2.4);
}
static void rgbToXyz(int r, int g, int b, double &x, double &y, double &z) {
    double rl = srgbToLinear(r), gl = srgbToLinear(g), bl = srgbToLinear(b);
    x = rl * 0.4124564 + gl * 0.3575761 + bl * 0.1804375;
    y = rl * 0.2126729 + gl * 0.7151522 + bl * 0.0721750;
    z = rl * 0.0193339 + gl * 0.1191920 + bl * 0.9503041;
}
static double labF(double v) {
    double d = 6.0/29.0;
    if (v > d*d*d) return std::cbrt(v);
    return v / (3*d*d) + 4.0/29.0;
}
static void rgbToLab(int r, int g, int b, double &L, double &a, double &b_) {
    double x,y,z; rgbToXyz(r,g,b,x,y,z);
    x/=0.95047; y/=1.0; z/=1.08883;
    double fx=labF(x), fy=labF(y), fz=labF(z);
    L = 116*fy -16;
    a = 500*(fx - fy);
    b_ = 200*(fy - fz);
}
static double labDist(double L1,double a1,double b1,double L2,double a2,double b2){
    double dl=L1-L2, da=a1-a2, db=b1-b2;
    return std::sqrt(dl*dl+da*da+db*db);
}
static QMap<QString, std::tuple<double,double,double>> groupLab() {
    QMap<QString, std::tuple<double,double,double>> m;
    for (auto it=kColorGroups.begin(); it!=kColorGroups.end(); ++it) {
        QString hex=it.value().hex;
        bool ok; int r=hex.mid(1,2).toInt(&ok,16); int g=hex.mid(3,2).toInt(&ok,16); int b=hex.mid(5,2).toInt(&ok,16);
        double L,a,bb; rgbToLab(r,g,b,L,a,bb); m[it.key()]={L,a,bb};
    }
    return m;
}
static QMap<QString, std::tuple<double,double,double>> gLab = groupLab();

QString classifyRgb(int r, int g, int b) {
    double L,a,bb; rgbToLab(r,g,b,L,a,bb);
    double chroma = std::sqrt(a*a + bb*bb);
    if (L < 15) { if (chroma < 4) return "gray"; }
    else if (L < 30) { if (chroma < 6) return "gray"; }
    else if (L < 70) { if (chroma < 9) return "gray"; }
    else { if (chroma < 11) return "gray"; }
    QString best; double bestDist=1e9;
    for (auto it=gLab.begin(); it!=gLab.end(); ++it) {
        if (it.key()=="gray") continue;
        auto [gl,ga,gb]=it.value();
        double d=labDist(L,a,bb,gl,ga,gb);
        if (d<bestDist){ bestDist=d; best=it.key(); }
    }
    return best;
}
QString getGroupColor(const QString &name){ auto it=kColorGroups.find(name); return it!=kColorGroups.end()? it.value().hex : "#9E9E9E"; }
