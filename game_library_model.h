#pragma once

#include <QAbstractListModel>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QHash>
#include <QSet>
#include "game_entry.h"
#include "trailer_cache.h"

class LibrarySyncWorker;

// The single source of truth QML binds to. Owns a background thread running
// LibrarySyncWorker; this class itself does no blocking work and no network
// or filesystem I/O -- it just holds the merged game list and reacts to
// signals from the worker.
class GameLibraryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool syncing READ isSyncing NOTIFY syncingChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // Top candidates for the hero banner -- most recently played first,
    // falling back to most-played for anything never launched via Steam.
    // Recomputed automatically as games and their art arrive.
    Q_PROPERTY(QVariantList heroGames READ heroGames NOTIFY heroGamesChanged)

public:
    enum Roles {
        AppIdRole = Qt::UserRole + 1,
        NameRole,
        PlaytimeRole,
        InstalledRole,
        BoxArtPathRole,
    };

    explicit GameLibraryModel(QObject *parent = nullptr);
    ~GameLibraryModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isSyncing() const { return m_syncing; }
    int count() const { return m_games.size(); }
    QVariantList heroGames() const { return m_heroGames; }

public slots:
    // Callable from QML (e.g. a Refresh button) as well as internally.
    // No-op if a sync is already in progress.
    void refresh();

    // Returns the full row at index as a map, for QML code that needs an
    // arbitrary row's data outside of a delegate context (e.g. the detail
    // panel, which shows whichever game has been selected for 2+ seconds).
    // Empty map if index is out of range.
    QVariantMap gameAt(int index) const;

    // Same as gameAt() but by appId instead of row index -- for QML code
    // (like the expanded detail view) that only has an appId to go on.
    // Empty map if not found.
    QVariantMap gameByAppId(qint64 appId) const;

    // Cached store details (description, requirements) for appId, or an
    // empty map if not yet fetched/requested. See requestStoreDetails().
    QVariantMap storeDetailsFor(qint64 appId) const;

    // Fetches store details for appId on a background thread if not already
    // cached or in flight. No-op otherwise. Emits storeDetailsReady(appId)
    // on completion.
    void requestStoreDetails(qint64 appId);

    // Cached ProtonDB compatibility rating for appId, or an empty map if
    // not yet fetched/requested. See requestProtonDbRating().
    QVariantMap protonDbRatingFor(qint64 appId) const;

    // Fetches ProtonDB's rating for appId on a background thread if not
    // already cached or in flight. No-op otherwise. Emits
    // protonDbRatingReady(appId) on completion. Same lazy-fetch-then-cache
    // shape as requestStoreDetails() -- meant to be called once when a
    // game's detail view is opened, not proactively for the whole library.
    void requestProtonDbRating(qint64 appId);

    // Local cache file (file:// URL) for appId's trailer, if it's already
    // been fully downloaded, or empty if not cached yet. See
    // requestTrailerCache(). QML should prefer this over the remote
    // trailer URL from store details whenever it's non-empty.
    QString cachedTrailerUrlFor(qint64 appId) const;

    // Starts downloading remoteUrl into the local trailer cache for appId
    // in the background, if not already cached or in flight. Requires
    // `ffmpeg` on PATH -- silently does nothing if it isn't available, so
    // this is always safe to call speculatively. Emits
    // trailerCached(appId) once the local copy is ready to use.
    void requestTrailerCache(qint64 appId, const QString &remoteUrl);

    // Launches an already-installed game via Steam's `-applaunch` CLI flag
    // rather than the steam://rungameid/ URL scheme -- skips the extra
    // hand-off-to-whatever's-registered-for-steam:// hop that URL launches
    // go through. Returns false (and does nothing) if a `steam` executable
    // isn't found on PATH, so the caller can fall back to the URL scheme.
    bool launchGameViaSteamCli(qint64 appId) const;

signals:
    void syncingChanged();
    void syncFailed(const QString &error);
    void countChanged();
    void heroGamesChanged();
    void storeDetailsReady(qint64 appId);
    void protonDbRatingReady(qint64 appId);
    void trailerCached(qint64 appId);

private slots:
    void onGameReady(const GameEntry &entry);
    void onInstallStateChanged();
    void onSyncFinished();

private:
    QList<GameEntry> m_games;
    QVariantList m_heroGames;
    QThread m_workerThread;
    LibrarySyncWorker *m_worker = nullptr;
    bool m_syncing = false;

    int indexForAppId(qint64 appId) const;
    void recomputeHeroGames();

    // Picks 5 random games from the library once (right after the first
    // sync finishes) and kicks off background trailer caching for them, so
    // browsing feels snappier without waiting for the player to open each
    // detail view first. Random per app launch -- stays varied session to
    // session -- and only ever runs once, so it doesn't re-fire on every
    // re-sync triggered by install-state changes.
    void warmRandomTrailers();

    QHash<qint64, QVariantMap> m_storeDetailsCache;
    QSet<qint64> m_storeDetailsInFlight;

    QHash<qint64, QVariantMap> m_protonDbCache;
    QSet<qint64> m_protonDbInFlight;

    TrailerCache m_trailerCache;

    bool m_trailerWarmupDone = false;
    QSet<qint64> m_warmupAppIds;
};
