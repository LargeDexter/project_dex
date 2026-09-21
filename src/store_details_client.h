#pragma once

#include <QObject>
#include <QVariantMap>

class StoreDetailsClient : public QObject
{
    Q_OBJECT

public:
    explicit StoreDetailsClient(QObject *parent = nullptr) : QObject(parent) {}

    // Blocking (uses a local event loop) -- call this from a background
    // thread, never the UI thread. Returns a map with:
    //   "available" (bool) -- false if Steam has no store page data for
    //       this app (common for tools, redistributables, dedicated
    //       servers). If false, the other keys are absent.
    //   "shortDescription" (string)
    //   "longDescription" (string, HTML fragment, possibly empty) -- the
    //       fuller "About This Game" write-up, for a "Show more" expansion
    //       beyond shortDescription.
    //   "minimumRequirements" / "recommendedRequirements" (string, HTML
    //       fragments as Steam provides them -- render with
    //       textFormat: Text.RichText)
    //   "trailerUrl" (string, possibly empty) -- a playable URL for the
    //       store page's highlighted trailer/movie, or the first movie
    //       listed if none is flagged as the highlight. Usually an HLS
    //       (.m3u8) or DASH (.mpd) manifest URL these days rather than a
    //       flat .mp4 -- Qt Multimedia's FFmpeg backend plays either
    //       directly as a MediaPlayer source. Empty if the app has no
    //       movies at all.
    //   "genres" (QStringList) -- Steam's genre tags for this app (e.g.
    //       "Action", "Adventure", "RPG"). Possibly empty.
    //   "categories" (QStringList) -- Steam's category tags (e.g.
    //       "Co-op", "Multi-player", "Single-player", "Controller
    //       Support"). Possibly empty. Used alongside genres to build the
    //       browsing rows (e.g. "Co-op" as its own row).
    QVariantMap fetchDetails(qint64 appId, QString &errorOut);
};
