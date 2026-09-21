// (mirrors store_details_client.h's shape/usage pattern deliberately, so
// GameLibraryModel's caching code for the two looks the same)
#pragma once

#include <QObject>
#include <QVariantMap>

// Fetches ProtonDB's crowd-sourced Linux/Proton compatibility rating for a
// Steam app. ProtonDB (https://www.protondb.com/) is an independent
// community project, unaffiliated with Valve or us -- this hits its public,
// unauthenticated summary endpoint, so no API key is needed.
class ProtonDbClient : public QObject
{
    Q_OBJECT

public:
    explicit ProtonDbClient(QObject *parent = nullptr) : QObject(parent) {}

    // Blocking (uses a local event loop) -- call this from a background
    // thread, never the UI thread. Returns a map with:
    //   "available" (bool) -- false if ProtonDB has no reports for this
    //       app yet (common for very new or very obscure games), or the
    //       request failed. If false, the other keys are absent/empty.
    //   "tier" (string) -- one of ProtonDB's rating tiers, lowercase,
    //       straight from the API: "borked", "bronze", "silver", "gold",
    //       "platinum", "native", or "pending" (not enough reports yet
    //       for a confident tier).
    //   "confidence" (string) -- ProtonDB's own confidence label for the
    //       tier (e.g. "strong").
    //   "reportCount" (int) -- number of user reports the tier is based
    //       on.
    QVariantMap fetchRating(qint64 appId, QString &errorOut);
};
