#include "app_settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

QString AppSettings::filePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/settings.json");
}

AppSettingsData AppSettings::load()
{
    AppSettingsData data;
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly))
        return data; // No saved settings yet -- empty is the correct "not configured" state.

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return data;

    const QJsonObject obj = doc.object();
    data.steamApiKey = obj.value(QStringLiteral("steamApiKey")).toString();
    data.steamId = obj.value(QStringLiteral("steamId")).toString();
    data.steamGridDbKey = obj.value(QStringLiteral("steamGridDbKey")).toString();
    // Defaults to true (see app_settings.h) for anyone who never touched
    // this setting, including existing settings.json files from before it
    // existed -- toBool(true) only falls back to that default when the key
    // is genuinely absent, not when it's present-and-false.
    data.fullscreen = obj.value(QStringLiteral("fullscreen")).toBool(true);
    return data;
}

bool AppSettings::save(const AppSettingsData &data, QString *errorOut)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("steamApiKey"), data.steamApiKey);
    obj.insert(QStringLiteral("steamId"), data.steamId);
    obj.insert(QStringLiteral("steamGridDbKey"), data.steamGridDbKey);
    obj.insert(QStringLiteral("fullscreen"), data.fullscreen);

    const QString path = filePath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (errorOut)
            *errorOut = QStringLiteral("Couldn't create config directory: %1").arg(QFileInfo(path).absolutePath());
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorOut)
            *errorOut = QStringLiteral("Couldn't write %1: %2").arg(path, file.errorString());
        return false;
    }

    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}
