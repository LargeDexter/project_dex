#pragma once

#include <QObject>
#include <QString>

class ArtworkClient : public QObject
{
    Q_OBJECT

public:
    // apiKey may be empty -- if so, SteamGridDB is skipped and the client
    // goes straight to the Steam CDN fallback for every request.
    explicit ArtworkClient(QString steamGridDbApiKey, QObject *parent = nullptr);

    // Resolves a box-art image URL for the given Steam appid: tries
    // SteamGridDB first (better curated art, multiple styles), falls back to
    // Steam's own store CDN if SteamGridDB has no key, no match, or errors.
    // Returns empty string and fills errorOut only if BOTH sources fail.
    QString resolveBoxArtUrl(qint64 steamAppId, bool &usedFallback, QString &errorOut);

    // Full pipeline: resolves the URL, downloads the image, and writes it to
    // the local cache directory (~/.cache/project-dex/boxart/<appid>.jpg).
    // Returns the local file path on success, empty string on failure.
    // usedFallbackOut reports whether SteamGridDB succeeded (false) or the
    // Steam CDN fallback was used instead (true).
    QString fetchAndCacheBoxArt(qint64 steamAppId, const QString &gameName,
                                 bool &usedFallbackOut, QString &errorOut);

private:
    QString m_apiKey;

    QString trySteamGridDb(qint64 steamAppId, QString &errorOut);
    QString steamCdnFallbackUrl(qint64 steamAppId) const;
    QString cacheDirPath() const;
};
