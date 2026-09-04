#include "Config.h"
Config::Config(const QJsonObject &data, QObject *parent) : QObject(parent), obj(data) {}
