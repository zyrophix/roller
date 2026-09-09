#include "AppConfig.h"
AppConfig::AppConfig(const QJsonObject &data, QObject *parent) : QObject(parent), obj(data) {}
