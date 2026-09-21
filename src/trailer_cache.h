#pragma once

#include <QObject>
#include <QSet>
#include <QString>

// Downloads and locally caches trailer videos the first time they're
// viewed, so re-opening the same game later starts playback instantly from
// disk instead of re-streaming (and re-buffering) from Steam's CDN every
// single time.
//
// Uses the system `ffmpeg` command-line tool as a subprocess to remux the
// HLS/DASH stream into a plain local .mp4 (no re-encoding -- just
// repackaging the already-encoded video/audio into a container a local
// file read can serve instantly). Qt Multimedia links FFmpeg's *libraries*
// internally for playback, but that doesn't include the `ffmpeg` binary
// itself -- install it separately (the `ffmpeg` package, e.g. via RPM
// Fusion on Fedora/Bazzite) to enable caching. Without it, trailers simply
// keep streaming from the network as before -- caching is a bonus on top
// of playback, never a requirement for it.
class TrailerCache : public QObject
{
    Q_OBJECT

public:
    explicit TrailerCache(QObject *parent = nullptr);

    // The local cache file for appId, as a file:// URL ready to hand
    // straight to a QML MediaPlayer's `source`, if it's already been fully
    // downloaded. Empty string if not cached yet.
    QString cachedTrailerUrl(qint64 appId) const;

    // Starts downloading remoteUrl to the local cache for appId in the
    // background, if it isn't already cached or already in flight, and if
    // ffmpeg is available. No-op otherwise. Emits trailerCached(appId) on
    // success; does nothing further on failure (the stream just keeps
    // playing from the network, exactly as if caching didn't exist).
    void requestCache(qint64 appId, const QString &remoteUrl);

signals:
    void trailerCached(qint64 appId);

private:
    QString cacheDir() const;
    QString cacheFilePath(qint64 appId) const;

    QSet<qint64> m_inFlight;
    bool m_ffmpegAvailable;
    QString m_ffmpegPath;
};
