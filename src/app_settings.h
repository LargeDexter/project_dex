#pragma once

#include <QString>

// The three credentials the app needs and can't ship with: a Steam Web API
// key + the user's own SteamID64 (required -- without these, LibrarySyncWorker
// can't fetch an owned-games list at all), and a SteamGridDB API key
// (optional -- ArtworkClient just falls back to the Steam CDN without one).
struct AppSettingsData
{
    QString steamApiKey;
    QString steamId;
    QString steamGridDbKey;
};

// Plain, non-QObject persistence for the above -- deliberately not part of
// SettingsManager (the QML-facing singleton) so it can be called from
// LibrarySyncWorker's background thread too, without any cross-thread
// Q_PROPERTY access. Same on-disk-JSON-under-AppConfigLocation shape as
// GamepadInput's own mapping file (see gamepad_input.cpp's
// mappingFilePath()/loadMapping()/saveMapping()) -- consistent with how
// this app already persists small bits of user config.
namespace AppSettings
{
QString filePath();
AppSettingsData load();
// Returns false and fills errorOut on failure (e.g. can't create/write the
// config directory) -- callers should surface that rather than silently
// losing the user's input.
bool save(const AppSettingsData &data, QString *errorOut = nullptr);
}
