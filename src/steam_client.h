#pragma once

#include <QObject>
#include <QString>
#include <QList>

struct OwnedGame {
    qint64 appId = 0;
    QString name;
    qint64 playtimeForeverMinutes = 0;
    qint64 rtimeLastPlayed = 0; // unix timestamp, 0 if never played
};

class SteamClient : public QObject
{
    Q_OBJECT

public:
    explicit SteamClient(QString apiKey, QString steamId, QObject *parent = nullptr);

    // Fetches owned games synchronously (blocks on a local event loop).
    // Returns true on success and fills outGames; on failure fills errorOut.
    bool fetchOwnedGames(QList<OwnedGame> &outGames, QString &errorOut);

private:
    QString m_apiKey;
    QString m_steamId;
};
