#include "artwork_client.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

ArtworkClient::ArtworkClient(QString steamGridDbApiKey, QObject *parent)
    : QObject(parent), m_apiKey(std::move(steamGridDbApiKey))
{
}

QString ArtworkClient::steamCdnFallbackUrl(qint64 steamAppId) const
{
    // Steam's own store CDN. library_600x900.jpg is the vertical "library
    // capsule" art shown in the Steam client itself -- same aspect ratio as
    // SteamGridDB's default grid style, so either source drops into the same
    // UI slot without layout changes.
    return QString("https://cdn.akamai.steamstatic.com/steam/apps/%1/library_600x900.jpg")
        .arg(steamAppId);
}

QString ArtworkClient::trySteamGridDb(qint64 steamAppId, QString &errorOut)
{
    if (m_apiKey.isEmpty()) {
        errorOut = "No SteamGridDB API key set";
        return {};
    }

    QNetworkAccessManager manager;

    // Step 1: resolve Steam appid -> SteamGridDB's internal game id.
    QNetworkRequest gameRequest(
        QUrl(QString("https://www.steamgriddb.com/api/v2/games/steam/%1").arg(steamAppId)));
    gameRequest.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    QEventLoop loop1;
    QNetworkReply *gameReply = manager.get(gameRequest);
    QObject::connect(gameReply, &QNetworkReply::finished, &loop1, &QEventLoop::quit);
    loop1.exec();

    if (gameReply->error() != QNetworkReply::NoError) {
        errorOut = QString("SteamGridDB game lookup failed: %1").arg(gameReply->errorString());
        gameReply->deleteLater();
        return {};
    }

    const QJsonDocument gameDoc = QJsonDocument::fromJson(gameReply->readAll());
    gameReply->deleteLater();

    const QJsonObject gameObj = gameDoc.object();
    if (!gameObj.value("success").toBool()) {
        errorOut = "SteamGridDB has no entry for this appid";
        return {};
    }
    const qint64 sgdbId = gameObj.value("data").toObject().value("id").toVariant().toLongLong();

    // Step 2: fetch grid (poster-style) art for that game id.
    QNetworkRequest gridRequest(QUrl(
        QString("https://www.steamgriddb.com/api/v2/grids/game/%1?dimensions=600x900").arg(sgdbId)));
    gridRequest.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    QEventLoop loop2;
    QNetworkReply *gridReply = manager.get(gridRequest);
    QObject::connect(gridReply, &QNetworkReply::finished, &loop2, &QEventLoop::quit);
    loop2.exec();

    if (gridReply->error() != QNetworkReply::NoError) {
        errorOut = QString("SteamGridDB grid fetch failed: %1").arg(gridReply->errorString());
        gridReply->deleteLater();
        return {};
    }

    const QJsonDocument gridDoc = QJsonDocument::fromJson(gridReply->readAll());
    gridReply->deleteLater();

    const QJsonObject gridObj = gridDoc.object();
    if (!gridObj.value("success").toBool()) {
        errorOut = "SteamGridDB returned no grid images for this game";
        return {};
    }

    const QJsonArray images = gridObj.value("data").toArray();
    if (images.isEmpty()) {
        errorOut = "SteamGridDB has the game but no 600x900 grid art";
        return {};
    }

    return images.first().toObject().value("url").toString();
}

QString ArtworkClient::resolveBoxArtUrl(qint64 steamAppId, bool &usedFallback, QString &errorOut)
{
    usedFallback = false;

    QString sgdbError;
    const QString sgdbUrl = trySteamGridDb(steamAppId, sgdbError);
    if (!sgdbUrl.isEmpty())
        return sgdbUrl;

    // Fall back to Steam's CDN. We don't verify the URL resolves here --
    // that happens naturally when fetchAndCacheBoxArt() downloads it.
    usedFallback = true;
    if (!sgdbError.isEmpty())
        errorOut = sgdbError; // kept for logging even though we're falling back
    return steamCdnFallbackUrl(steamAppId);
}

QString ArtworkClient::cacheDirPath() const
{
    // Deliberately not QStandardPaths::CacheLocation -- that folds in the
    // running executable's name (sync_test, artwork_test, project_dex, ...),
    // which would give every binary its own separate cache instead of one
    // shared "project-dex" cache. Anchor directly off $HOME/.cache instead.
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    return cacheRoot + "/project-dex/boxart";
}

QString ArtworkClient::fetchAndCacheBoxArt(qint64 steamAppId, const QString &gameName,
                                            bool &usedFallbackOut, QString &errorOut)
{
    Q_UNUSED(gameName);

    const QDir dir(cacheDirPath());
    const QString localPath = dir.filePath(QString("%1.jpg").arg(steamAppId));

    // Already cached from a previous sync -- skip the network round-trip
    // entirely. This is what makes repeat syncs (e.g. after an install
    // state change) fast instead of re-downloading 60+ images every time.
    if (QFile::exists(localPath)) {
        usedFallbackOut = false;
        return localPath;
    }

    QString resolveError;
    const QString imageUrl = resolveBoxArtUrl(steamAppId, usedFallbackOut, resolveError);
    if (imageUrl.isEmpty()) {
        errorOut = resolveError.isEmpty() ? "No box art URL could be resolved" : resolveError;
        return {};
    }

    QNetworkAccessManager manager;
    QNetworkRequest request((QUrl(imageUrl)));

    QEventLoop loop;
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        errorOut = QString("Image download failed: %1").arg(reply->errorString());
        reply->deleteLater();
        return {};
    }

    const QByteArray imageBytes = reply->readAll();
    reply->deleteLater();

    if (!dir.exists())
        QDir().mkpath(dir.absolutePath());

    QFile outFile(localPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        errorOut = "Could not write to cache directory: " + localPath;
        return {};
    }
    outFile.write(imageBytes);
    outFile.close();

    return localPath;
}
