#include "library_sync_worker.h"
#include "steam_client.h"
#include "library_scanner.h"
#include "artwork_client.h"
#include "app_settings.h"

#include <QProcessEnvironment>
#include <QSet>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureSynchronizer>

LibrarySyncWorker::LibrarySyncWorker(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<GameEntry>();
}

LibrarySyncWorker::~LibrarySyncWorker() = default;

void LibrarySyncWorker::startSync()
{
    // The setup wizard (SettingsManager/AppSettings) is the primary source
    // now -- STEAM_API_KEY/STEAM_ID/STEAMGRIDDB_API_KEY env vars are kept
    // only as a fallback for whichever of the three isn't set there, so
    // this doesn't break for anyone (namely, during earlier development)
    // who's been relying on exporting them instead.
    const AppSettingsData settings = AppSettings::load();
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString steamApiKey = settings.steamApiKey.isEmpty() ? env.value("STEAM_API_KEY") : settings.steamApiKey;
    const QString steamId = settings.steamId.isEmpty() ? env.value("STEAM_ID") : settings.steamId;
    const QString gridDbKey = settings.steamGridDbKey.isEmpty() ? env.value("STEAMGRIDDB_API_KEY") : settings.steamGridDbKey; // may still end up empty -- artwork client falls back to Steam CDN

    if (steamApiKey.isEmpty() || steamId.isEmpty()) {
        // A fixed sentinel rather than a human sentence -- GameLibraryModel
        // just forwards this signal as-is, and Main.qml matches on this
        // exact string to know to auto-open the setup wizard, as opposed to
        // a genuine network/API failure it should just leave visible.
        emit syncFailed(QStringLiteral("missing-credentials"));
        emit syncFinished();
        return;
    }

    // Rebuilt every sync (not lazily cached like m_libraryScanner below) --
    // the whole point of the setup wizard is that these keys can change
    // mid-session, and a stale SteamClient/ArtworkClient built from
    // whatever keys happened to be set the *first* time startSync() ran
    // would silently keep using them on every refresh after that.
    delete m_steamClient;
    m_steamClient = new SteamClient(steamApiKey, steamId, this);
    delete m_artworkClient;
    m_artworkClient = new ArtworkClient(gridDbKey, this);

    if (!m_libraryScanner)
        m_libraryScanner = new LibraryScanner(this);

    QList<OwnedGame> owned;
    QString ownedError;
    if (!m_steamClient->fetchOwnedGames(owned, ownedError)) {
        emit syncFailed("Steam sync failed: " + ownedError);
        emit syncFinished();
        return;
    }

    QSet<qint64> installedIds;
    if (m_libraryScanner->discoverLibraries()) {
        const QList<InstalledApp> installedApps = m_libraryScanner->scanInstalledApps();
        for (const InstalledApp &a : installedApps)
            installedIds.insert(a.appId);

        if (!m_watchingStarted) {
            connect(m_libraryScanner, &LibraryScanner::installChanged,
                    this, &LibrarySyncWorker::installStateChanged);
            m_libraryScanner->startWatching();
            m_watchingStarted = true;
        }
    }

    // Phase 1: build and emit every game immediately, art still empty.
    // This is what makes the grid populate instantly instead of trickling
    // in one game at a time.
    QList<GameEntry> entries;
    entries.reserve(owned.size());
    for (const OwnedGame &g : owned) {
        GameEntry entry;
        entry.appId = g.appId;
        entry.name = g.name;
        entry.playtimeMinutes = g.playtimeForeverMinutes;
        entry.rtimeLastPlayed = g.rtimeLastPlayed;
        entry.installed = installedIds.contains(g.appId);
        entries.append(entry);
        emit gameReady(entry);
    }

    // Phase 2: fetch box art for all of them concurrently (bounded pool, so
    // we don't hammer SteamGridDB's API), emitting an updated GameEntry as
    // each one completes. Already-cached art (see ArtworkClient) returns
    // near-instantly with no network call, so repeat syncs are fast.
    QThreadPool artPool;
    artPool.setMaxThreadCount(4);

    QFutureSynchronizer<void> synchronizer;
    for (const GameEntry &entry : entries) {
        QFuture<void> future = QtConcurrent::run(&artPool, [this, entry]() {
            GameEntry updated = entry;
            QString artError;
            bool usedFallback = false;
            updated.boxArtPath =
                m_artworkClient->fetchAndCacheBoxArt(entry.appId, entry.name, usedFallback, artError);
            emit gameReady(updated);
        });
        synchronizer.addFuture(future);
    }
    synchronizer.waitForFinished(); // blocks this worker-thread slot only, not the UI thread

    emit syncFinished();
}
