#include "proton_db_client.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

QVariantMap ProtonDbClient::fetchRating(qint64 appId, QString &errorOut)
{
    QVariantMap result;
    result["available"] = false;

    const QUrl url(QStringLiteral("https://www.protondb.com/api/v1/reports/summaries/%1.json")
                        .arg(appId));

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setTransferTimeout(8000); // ms -- Qt's own timeout mechanism
    request.setHeader(QNetworkRequest::UserAgentHeader,
                       "Mozilla/5.0 (X11; Linux x86_64) ProjectDex/1.0");

    QEventLoop loop;
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // Hard fallback, same reasoning as StoreDetailsClient: guarantees the
    // loop exits even if setTransferTimeout doesn't behave as expected.
    QTimer fallbackTimer;
    fallbackTimer.setSingleShot(true);
    QObject::connect(&fallbackTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    fallbackTimer.start(10000);

    loop.exec();

    if (!reply->isFinished()) {
        errorOut = "Request timed out";
        reply->abort();
        reply->deleteLater();
        return result;
    }

    if (reply->error() != QNetworkReply::NoError) {
        // A 404 just means ProtonDB has no reports for this app at all --
        // common for very new/obscure games, and not a real error, so it's
        // not surfaced as one; the caller just gets available: false.
        if (reply->error() != QNetworkReply::ContentNotFoundError)
            errorOut = reply->errorString();
        reply->deleteLater();
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    const QJsonObject obj = doc.object();
    const QString tier = obj.value("tier").toString();
    if (tier.isEmpty())
        return result;

    result["available"] = true;
    result["tier"] = tier;
    result["confidence"] = obj.value("confidence").toString();
    result["reportCount"] = obj.value("total").toInt();
    return result;
}
