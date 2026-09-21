#include "settings_manager.h"
#include "app_settings.h"

#include <QRegularExpression>

SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
    const AppSettingsData data = AppSettings::load();
    m_steamApiKey = data.steamApiKey;
    m_steamId = data.steamId;
    m_steamGridDbKey = data.steamGridDbKey;
}

bool SettingsManager::qrCodeAvailable() const
{
#ifdef PROJECT_DEX_HAS_QRENCODE
    return true;
#else
    return false;
#endif
}

QString SettingsManager::save(const QString &steamApiKey, const QString &steamId, const QString &steamGridDbKey)
{
    const QString trimmedApiKey = steamApiKey.trimmed();
    const QString trimmedSteamId = steamId.trimmed();
    const QString trimmedGridDbKey = steamGridDbKey.trimmed();

    AppSettingsData data;
    data.steamApiKey = trimmedApiKey;
    data.steamId = trimmedSteamId;
    data.steamGridDbKey = trimmedGridDbKey;

    QString error;
    if (!AppSettings::save(data, &error))
        return error.isEmpty() ? QStringLiteral("Couldn't save settings.") : error;

    // Named *Differs rather than *Changed -- a local bool named the same
    // as a signal (steamIdChanged, right below) shadows the signal inside
    // this function, so `emit steamIdChanged()` would resolve to the bool
    // instead of calling the signal, and fail to compile.
    const bool apiKeyDiffers = trimmedApiKey != m_steamApiKey;
    const bool steamIdDiffers = trimmedSteamId != m_steamId;
    const bool gridDbKeyDiffers = trimmedGridDbKey != m_steamGridDbKey;
    const bool hadCredentialsBefore = hasCredentials();

    m_steamApiKey = trimmedApiKey;
    m_steamId = trimmedSteamId;
    m_steamGridDbKey = trimmedGridDbKey;

    if (apiKeyDiffers)
        emit steamApiKeyChanged();
    if (steamIdDiffers)
        emit steamIdChanged();
    if (gridDbKeyDiffers)
        emit steamGridDbKeyChanged();
    if (hadCredentialsBefore != hasCredentials())
        emit hasCredentialsChanged();

    return QString(); // empty = success
}

QString SettingsManager::validateSteamApiKey(const QString &key) const
{
    const QString trimmed = key.trimmed();
    if (trimmed.isEmpty())
        return QStringLiteral("Required to sync your library.");
    // Steam Web API keys are always exactly 32 hex characters.
    static const QRegularExpression pattern(QStringLiteral("^[0-9A-Fa-f]{32}$"));
    if (!pattern.match(trimmed).hasMatch())
        return QStringLiteral("Should be 32 letters/numbers (A-F, 0-9) -- check for a typo or extra space.");
    return QString();
}

QString SettingsManager::validateSteamId(const QString &steamId) const
{
    const QString trimmed = steamId.trimmed();
    if (trimmed.isEmpty())
        return QStringLiteral("Required to sync your library.");
    // SteamID64s are 17-digit numbers, all currently starting with 7656119
    // (Steam's fixed account-type/instance prefix for individual accounts).
    // Checking the exact prefix (rather than just "17 digits") catches the
    // single most likely mix-up: pasting a profile URL's vanity name or a
    // SteamID3/SteamID32 instead of the 64-bit ID this app actually needs.
    static const QRegularExpression pattern(QStringLiteral("^7656119\\d{10}$"));
    if (!pattern.match(trimmed).hasMatch())
        return QStringLiteral("Should be your 17-digit SteamID64 (starts with 7656119...).");
    return QString();
}

QString SettingsManager::validateSteamGridDbKey(const QString &key) const
{
    const QString trimmed = key.trimmed();
    if (trimmed.isEmpty())
        return QString(); // Optional -- ArtworkClient falls back to the Steam CDN without one.
    if (trimmed.size() < 16)
        return QStringLiteral("That looks too short for a SteamGridDB key -- check for a typo.");
    return QString();
}
