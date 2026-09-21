#include "steam_client.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

SteamClient::SteamClient(QString apiKey, QString steamId, QObject *parent)
    : QObject(parent), m_apiKey(std::move(apiKey)), m_steamId(std::move(steamId))
{
}

bool SteamClient::fetchOwnedGames(QList<OwnedGame> &outGames, QString &errorOut)
{
    QUrl url("https://api.steampowered.com/IPlayerService/GetOwnedGames/v0001/");
    QUrlQuery query;
    query.addQueryItem("key", m_apiKey);
    query.addQueryItem("steamid", m_steamId);
    query.addQueryItem("format", "json");
    query.addQueryItem("include_appinfo", "1");
    query.addQueryItem("include_played_free_games", "1");
    url.setQuery(query);

    QNetworkAccessManager manager;
    QNetworkRequest request(url);

    QEventLoop loop;
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        errorOut = QString("Network error: %1").arg(reply->errorString());
        reply->deleteLater();
        return false;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errorOut = QString("Failed to parse JSON response: %1").arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = doc.object().value("response").toObject();
    if (!root.contains("games")) {
        errorOut = "Response had no 'games' field -- check that the API key and SteamID64 are correct "
                    "and that the Steam profile's game details are set to Public.";
        return false;
    }

    const QJsonArray games = root.value("games").toArray();
    outGames.clear();
    outGames.reserve(games.size());

    for (const QJsonValue &v : games) {
        const QJsonObject g = v.toObject();
        OwnedGame game;
        game.appId = g.value("appid").toVariant().toLongLong();
        game.name = g.value("name").toString();
        game.playtimeForeverMinutes = g.value("playtime_forever").toVariant().toLongLong();
        game.rtimeLastPlayed = g.value("rtime_last_played").toVariant().toLongLong();
        outGames.append(game);
    }

    return true;
}
