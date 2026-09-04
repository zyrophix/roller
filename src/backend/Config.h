#pragma once
#include <QObject>
#include <QJsonObject>

class Config : public QObject {
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
    Q_PROPERTY(QString transitionType READ transitionType NOTIFY changed)
    Q_PROPERTY(QString transitionPos READ transitionPos NOTIFY changed)
    Q_PROPERTY(double transitionDuration READ transitionDuration NOTIFY changed)
    Q_PROPERTY(int transitionFps READ transitionFps NOTIFY changed)
public:
    explicit Config(const QJsonObject &data, QObject *parent=nullptr);
    QString borderColor() const { return obj.value("border_color").toString("#b4befe"); }
    int borderWidth() const { return obj.value("border_width").toInt(4); }
    int numberOfPictures() const { return obj.value("number_of_pictures").toInt(5); }
    int panelHeight() const { return obj.value("panel_height").toInt(500); }
    double horizontalScale() const { return obj.value("selected_horizontal_scale").toDouble(1.6); }
    double verticalScale() const { return obj.value("selected_vertical_scale").toDouble(1.1); }
    QString searchHintText() const { return obj.value("search_hint_text").toString("Press Ctrl + F or / to search"); }
    bool showSearchHint() const { return obj.value("show_search_hint").toBool(true); }
    QString searchBackgroundColor() const { return obj.value("search_background_color").toString("#313244"); }
    QString searchTextColor() const { return obj.value("search_text_color").toString("#cdd6f4"); }
    QString searchHintColor() const { return obj.value("search_hint_color").toString("#a6adc8"); }
    QString carouselSelectedBorder() const { return obj.value("carousel_selected_border").toString("#b4befe"); }
    QString transitionType() const { return obj.value("transition_type").toString("grow"); }
    QString transitionPos() const { return obj.value("transition_pos").toString("0.5,0.5"); }
    double transitionDuration() const { return obj.value("transition_duration").toDouble(1.2); }
    int transitionFps() const { return obj.value("transition_fps").toInt(60); }
    QJsonObject obj;
signals:
    void changed();
};
