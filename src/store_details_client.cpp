#include "store_details_client.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
// Picks a playable trailer URL out of Steam's "movies" array.
//
// Steam has served (at least) two different shapes for each entry over
// time:
//   Current:  { "id", "name", "thumbnail", "highlight": bool,
//               "dash_av1": "....mpd", "dash_h264": "....mpd",
//               "hls_h264": "....m3u8" }
//   Legacy:   { "id", "name", "thumbnail", "highlight": bool,
//               "mp4": { "480": "...", "max": "..." },
//               "webm": { "480": "...", "max": "..." } }
// (Confirmed by curling the live appdetails API in Sept 2026 -- the old
// flat mp4/webm shape is gone for at least some apps, replaced by DASH/HLS
// manifest URLs. Both are handled here since we can't be sure which shape
// any given app/region will return.)
//
// hls_h264 (.m3u8) is preferred: Qt Multimedia's FFmpeg backend (the
// default on Linux) has solid native HLS demuxing, whereas the DASH
// (.mpd) variants are pickier about player support. dash_h264 is tried
// next, then dash_av1 (needs an AV1 decoder -- not guaranteed on every
// GPU/driver combo), then the legacy mp4/webm fields as a last resort.
QString pickTrailerUrl(const QJsonArray &movies)
{
    if (movies.isEmpty())
        return QString();

    auto urlFromMovie = [](const QJsonObject &movie) -> QString {
        const QString hls = movie.value("hls_h264").toString();
        if (!hls.isEmpty())
            return hls;

        const QString dashH264 = movie.value("dash_h264").toString();
        if (!dashH264.isEmpty())
            return dashH264;

        const QString dashAv1 = movie.value("dash_av1").toString();
        if (!dashAv1.isEmpty())
            return dashAv1;

        const QJsonObject mp4 = movie.value("mp4").toObject();
        const QString mp4Max = mp4.value("max").toString();
        if (!mp4Max.isEmpty())
            return mp4Max;
        const QString mp4_480 = mp4.value("480").toString();
        if (!mp4_480.isEmpty())
            return mp4_480;

        const QJsonObject webm = movie.value("webm").toObject();
        const QString webmMax = webm.value("max").toString();
        if (!webmMax.isEmpty())
            return webmMax;
        return webm.value("480").toString();
    };

    for (const QJsonValue &v : movies) {
        const QJsonObject movie = v.toObject();
        if (movie.value("highlight").toBool()) {
            const QString url = urlFromMovie(movie);
            if (!url.isEmpty())
                return url;
        }
    }

    for (const QJsonValue &v : movies) {
        const QString url = urlFromMovie(v.toObject());
        if (!url.isEmpty())
            return url;
    }

    return QString();
}
} // namespace

QVariantMap StoreDetailsClient::fetchDetails(qint64 appId, QString &errorOut)
{
    QVariantMap result;

    QUrl url("https://store.steampowered.com/api/appdetails");
    QUrlQuery query;
    query.addQueryItem("appids", QString::number(appId));
    query.addQueryItem("cc", "us");
    query.addQueryItem("l", "english");
    url.setQuery(query);

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setTransferTimeout(8000); // ms -- Qt's own timeout mechanism
    request.setHeader(QNetworkRequest::UserAgentHeader,
                       "Mozilla/5.0 (X11; Linux x86_64) ProjectDex/1.0");

    QEventLoop loop;
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // Hard fallback: guarantees the loop exits even if setTransferTimeout
    // doesn't behave as expected on this Qt/platform combination -- without
    // this, a hung connection would block this worker thread (and this
    // fetch) forever, which is indistinguishable from "stuck loading" in
    // the UI.
    QTimer fallbackTimer;
    fallbackTimer.setSingleShot(true);
    QObject::connect(&fallbackTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    fallbackTimer.start(10000);

    loop.exec();

    if (!reply->isFinished()) {
        // The fallback timer fired -- request never completed.
        errorOut = "Request timed out";
        reply->abort();
        reply->deleteLater();
        result["available"] = false;
        return result;
    }

    if (reply->error() != QNetworkReply::NoError) {
        errorOut = reply->errorString();
        reply->deleteLater();
        result["available"] = false;
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    const QJsonObject root = doc.object();
    const QJsonObject appObj = root.value(QString::number(appId)).toObject();

    if (!appObj.value("success").toBool()) {
        // Common and expected for tools, redistributables, dedicated
        // servers, etc. -- not an error, just nothing to show.
        result["available"] = false;
        return result;
    }

    const QJsonObject data = appObj.value("data").toObject();
    result["available"] = true;
    result["shortDescription"] = data.value("short_description").toString();

    // The fuller "About This Game" write-up, for a "Show more" expansion
    // beyond the one-liner above. Falls back to detailed_description (an
    // older/alternate field Steam has used for roughly the same content)
    // if about_the_game isn't present. Both are HTML fragments, same as
    // the requirements fields.
    QString longDescription = data.value("about_the_game").toString();
    if (longDescription.isEmpty())
        longDescription = data.value("detailed_description").toString();
    result["longDescription"] = longDescription;

    // pc_requirements is sometimes an empty JSON array [] instead of an
    // object when Steam has nothing listed -- toObject() safely yields an
    // empty object in that case, so .value(...) below just comes back empty.
    const QJsonObject pcReq = data.value("pc_requirements").toObject();
    result["minimumRequirements"] = pcReq.value("minimum").toString();
    result["recommendedRequirements"] = pcReq.value("recommended").toString();

    result["trailerUrl"] = pickTrailerUrl(data.value("movies").toArray());

    // Genre/category tags, for grouping games into browsing rows (e.g. a
    // "Co-op" row or an "RPG" row). Both are arrays of {"id", "description"}
    // objects on Steam's side -- we only need the human-readable label.
    auto descriptionsOf = [](const QJsonArray &arr) {
        QStringList out;
        for (const QJsonValue &v : arr) {
            const QString desc = v.toObject().value("description").toString();
            if (!desc.isEmpty())
                out.append(desc);
        }
        return out;
    };
    result["genres"] = descriptionsOf(data.value("genres").toArray());
    result["categories"] = descriptionsOf(data.value("categories").toArray());

    return result;
}
