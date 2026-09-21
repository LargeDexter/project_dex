#include "trailer_cache.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

TrailerCache::TrailerCache(QObject *parent) : QObject(parent)
{
    m_ffmpegPath = QStandardPaths::findExecutable("ffmpeg");
    m_ffmpegAvailable = !m_ffmpegPath.isEmpty();
}

QString TrailerCache::cacheDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return base + "/trailers";
}

QString TrailerCache::cacheFilePath(qint64 appId) const
{
    return cacheDir() + "/" + QString::number(appId) + ".mp4";
}

QString TrailerCache::cachedTrailerUrl(qint64 appId) const
{
    const QString path = cacheFilePath(appId);
    if (QFile::exists(path))
        return QUrl::fromLocalFile(path).toString();
    return QString();
}

void TrailerCache::requestCache(qint64 appId, const QString &remoteUrl)
{
    if (!m_ffmpegAvailable || remoteUrl.isEmpty())
        return;
    if (m_inFlight.contains(appId) || QFile::exists(cacheFilePath(appId)))
        return;

    m_inFlight.insert(appId);

    const QString ffmpeg = m_ffmpegPath;
    const QString dir = cacheDir();
    const QString finalPath = cacheFilePath(appId);
    const QString tmpPath = finalPath + ".part";

    QtConcurrent::run(QThreadPool::globalInstance(), [this, appId, ffmpeg, dir, finalPath, tmpPath, remoteUrl]() {
        QDir().mkpath(dir);

        // -c copy: no re-encoding, just repackaging the already-encoded
        // H.264/AAC stream into a plain .mp4 container -- fast, and
        // doesn't burn CPU doing it. -y overwrites a stale leftover
        // partial file from a previous crashed/killed attempt.
        QProcess proc;
        proc.setProgram(ffmpeg);
        proc.setArguments({"-y", "-i", remoteUrl, "-c", "copy", tmpPath});
        proc.start();
        // Trailers are short (a minute or two) -- 90s is generous even on
        // a slow connection, and we'd rather give up than hang a
        // thread-pool thread indefinitely.
        const bool finished = proc.waitForFinished(90000);

        bool success = false;
        if (finished && proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0
            && QFile::exists(tmpPath)) {
            QFile::remove(finalPath);
            success = QFile::rename(tmpPath, finalPath);
        }
        if (!success)
            QFile::remove(tmpPath);

        QMetaObject::invokeMethod(this, [this, appId, success]() {
            m_inFlight.remove(appId);
            if (success)
                emit trailerCached(appId);
        }, Qt::QueuedConnection);
    });
}
