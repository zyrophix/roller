#pragma once
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>

class WallpaperRepository {
public:
    explicit WallpaperRepository(const QString &wallpaperDir);
    // Video extensions are configurable because they are the same list the
    // backend uses to decide that a file must go to mpvpaper; one source of
    // truth keeps the picker from showing something it cannot apply.
    void setVideoExtensions(const QStringList &exts) { videoExts = exts; }
    void refresh();
    QStringList getAll() const; // absolute paths

private:
    QString dir;
    QStringList wallpapers; // sorted absolute paths
    QStringList videoExts;
};
