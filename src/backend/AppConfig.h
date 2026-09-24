#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <cmath>

class AppConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString borderColor READ borderColor NOTIFY changed)
    Q_PROPERTY(int borderWidth READ borderWidth NOTIFY changed)
    Q_PROPERTY(int numberOfPictures READ numberOfPictures NOTIFY changed)
    Q_PROPERTY(int panelHeight READ panelHeight NOTIFY changed)
    Q_PROPERTY(double horizontalScale READ horizontalScale NOTIFY changed)
    Q_PROPERTY(double verticalScale READ verticalScale NOTIFY changed)
    Q_PROPERTY(QString searchHintText READ searchHintText NOTIFY changed)
    Q_PROPERTY(bool showSearchHint READ showSearchHint NOTIFY changed)
    Q_PROPERTY(QString searchBackgroundColor READ searchBackgroundColor NOTIFY changed)
    Q_PROPERTY(QString searchTextColor READ searchTextColor NOTIFY changed)
    Q_PROPERTY(QString searchHintColor READ searchHintColor NOTIFY changed)
    Q_PROPERTY(QString carouselSelectedBorder READ carouselSelectedBorder NOTIFY changed)
    Q_PROPERTY(int idleBorderWidth READ idleBorderWidth NOTIFY changed)
    Q_PROPERTY(QString idleBorderColor READ idleBorderColor NOTIFY changed)
    Q_PROPERTY(int thumbnailHeight READ thumbnailHeight NOTIFY changed)
    Q_PROPERTY(int cacheMaxMb READ cacheMaxMb NOTIFY changed)
    Q_PROPERTY(bool restoreLast READ restoreLast NOTIFY changed)
    Q_PROPERTY(QString lastWallpaper READ lastWallpaper NOTIFY changed)
    Q_PROPERTY(QString backendName READ backendName NOTIFY changed)
    Q_PROPERTY(QString backend READ backend NOTIFY changed)
    Q_PROPERTY(QString transitionType READ transitionType NOTIFY changed)
    Q_PROPERTY(QString transitionPos READ transitionPos NOTIFY changed)
    Q_PROPERTY(double transitionDuration READ transitionDuration NOTIFY changed)
    Q_PROPERTY(int transitionFps READ transitionFps NOTIFY changed)
    Q_PROPERTY(bool useLayerShell READ useLayerShell NOTIFY changed)
public:
    explicit AppConfig(const QJsonObject &data, QObject *parent=nullptr);
    QString borderColor() const { return obj.value("border_color").toString("#b4befe"); }
    int borderWidth() const { return obj.value("border_width").toInt(2); }
    int numberOfPictures() const { return qBound(1, obj.value("number_of_pictures").toInt(5), 64); }
    int panelHeight() const { return qBound(1, obj.value("panel_height").toInt(500), 2160); }
    double horizontalScale() const {
        double v = obj.value("selected_horizontal_scale").toDouble(1.6);
        return (std::isfinite(v) && v >= 1.0) ? qBound(1.0, v, 4.0) : 1.6;
    }
    double verticalScale() const {
        double v = obj.value("selected_vertical_scale").toDouble(1.1);
        return (std::isfinite(v) && v >= 1.0) ? qBound(1.0, v, 4.0) : 1.1;
    }
    QString searchHintText() const { return obj.value("search_hint_text").toString("Press Ctrl + F or / to search"); }
    bool showSearchHint() const { return obj.value("show_search_hint").toBool(true); }
    QString searchBackgroundColor() const { return obj.value("search_background_color").toString("#313244"); }
    QString searchTextColor() const { return obj.value("search_text_color").toString("#cdd6f4"); }
    QString searchHintColor() const { return obj.value("search_hint_color").toString("#a6adc8"); }
    QString carouselSelectedBorder() const { return obj.value("carousel_selected_border").toString("#b4befe"); }
    int idleBorderWidth() const { return obj.value("idle_border_width").toInt(2); }
    QString idleBorderColor() const { return obj.value("idle_border_color").toString("#585b70"); }
    // 512 is the freedesktop thumbnail tier and, at 16:9, keeps one decoded
    // ARGB32 thumb (910x512) under Qt's 2 MiB unreferenced pixmap cache
    // limit; 550 does not fit. Both the generator and the QML tiles must
    // agree on this number or they request different cache keys.
    int thumbnailHeight() const { return qBound(128, obj.value("thumbnail_height").toInt(512), 2160); }
    int cacheMaxMb() const { return qBound(16, obj.value("cache_max_mb").toInt(256), 8192); }
    bool restoreLast() const { return obj.value("restore_last").toBool(true); }

    // remembers the applied wallpaper so the picker opens on it again
    QString lastWallpaper() const;
    Q_INVOKABLE void saveLastWallpaper(const QString &path);
    void setStateFile(const QString &p) { stateFile = p; }
    QString backend() const { return obj.value("backend").toString("auto"); }
    QStringList videoExtensions() const {
        const auto a = obj.value("video_extensions").toArray();
        if (a.isEmpty()) return {};
        QStringList out;
        for (const auto &v : a) out << v.toString();
        return out;
    }
    QString stableCopyPath() const { return obj.value("stable_copy_path").toString(); }
    QString postApplyCommand() const { return obj.value("post_apply_command").toString(); }
    // canonical name of the backend that will actually be used
    QString backendName() const;
    QString transitionType() const { return obj.value("transition_type").toString("grow"); }
    QString transitionPos() const { return obj.value("transition_pos").toString("0.5,0.5"); }
    double transitionDuration() const { return obj.value("transition_duration").toDouble(1.2); }
    int transitionFps() const { return obj.value("transition_fps").toInt(60); }
    bool useLayerShell() const { return obj.value("use_layer_shell").toBool(false); }
    QJsonObject obj;
    QString stateFile;
signals:
    void changed();
};
