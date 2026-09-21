#include "game_library_model.h"
#include "library_sync_worker.h"
#include "store_details_client.h"
#include "proton_db_client.h"

#include <QVariantMap>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <QProcess>
#include <QStandardPaths>
#include <QRandomGenerator>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

GameLibraryModel::GameLibraryModel(QObject *parent) : QAbstractListModel(parent)
{
    m_worker = new LibrarySyncWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &LibrarySyncWorker::gameReady, this, &GameLibraryModel::onGameReady);
    connect(m_worker, &LibrarySyncWorker::installStateChanged, this, &GameLibraryModel::onInstallStateChanged);
    connect(m_worker, &LibrarySyncWorker::syncFinished, this, &GameLibraryModel::onSyncFinished);
    connect(m_worker, &LibrarySyncWorker::syncFailed, this, &GameLibraryModel::syncFailed);
    connect(&m_trailerCache, &TrailerCache::trailerCached, this, &GameLibraryModel::trailerCached);

    m_categorySeed = QRandomGenerator::global()->generate();

    m_categoryRecomputeTimer = new QTimer(this);
    m_categoryRecomputeTimer->setSingleShot(true);
    // Each recompute replaces categoryRows wholesale, which tears down and
    // rebuilds every category row's QML delegates (a Repeater with a plain
    // JS array model can't diff it and just recreates everything). During
    // the first several seconds after launch, storeDetailsReady fires once
    // per game as genre/category data streams in -- at 400ms this meant
    // dozens of full rebuilds back to back, which is what was behind the
    // startup flicker/focus-loss reports. 1200ms coalesces far more of
    // those into each rebuild while still filling rows in visibly over
    // the first several seconds rather than in one big jump at the end.
    m_categoryRecomputeTimer->setInterval(1200);
    connect(m_categoryRecomputeTimer, &QTimer::timeout, this, &GameLibraryModel::recomputeCategoryRows);
    connect(this, &GameLibraryModel::storeDetailsReady, this, [this](qint64) {
        m_categoryRecomputeTimer->start();
    });

    m_workerThread.start();
}

GameLibraryModel::~GameLibraryModel()
{
    m_workerThread.quit();
    m_workerThread.wait();
}

int GameLibraryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_games.size();
}

QVariant GameLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_games.size())
        return {};

    const GameEntry &g = m_games.at(index.row());
    switch (role) {
    case AppIdRole:      return g.appId;
    case NameRole:       return g.name;
    case PlaytimeRole:   return g.playtimeMinutes;
    case InstalledRole:  return g.installed;
    case BoxArtPathRole: return g.boxArtPath;
    default:             return {};
    }
}

QHash<int, QByteArray> GameLibraryModel::roleNames() const
{
    return {
        {AppIdRole, "appId"},
        {NameRole, "name"},
        {PlaytimeRole, "playtime"},
        {InstalledRole, "installed"},
        {BoxArtPathRole, "boxArtPath"},
    };
}

void GameLibraryModel::refresh()
{
    if (m_syncing)
        return;
    m_syncing = true;
    emit syncingChanged();
    QMetaObject::invokeMethod(m_worker, "startSync", Qt::QueuedConnection);
}

QVariantMap GameLibraryModel::gameAt(int index) const
{
    QVariantMap m;
    if (index < 0 || index >= m_games.size())
        return m;

    const GameEntry &g = m_games.at(index);
    m["appId"] = g.appId;
    m["name"] = g.name;
    m["playtime"] = g.playtimeMinutes;
    m["lastPlayed"] = g.rtimeLastPlayed;
    m["installed"] = g.installed;
    m["boxArtPath"] = g.boxArtPath;
    return m;
}

QVariantMap GameLibraryModel::gameByAppId(qint64 appId) const
{
    const int idx = indexForAppId(appId);
    return idx >= 0 ? gameAt(idx) : QVariantMap();
}

QVariantMap GameLibraryModel::storeDetailsFor(qint64 appId) const
{
    return m_storeDetailsCache.value(appId);
}

void GameLibraryModel::requestStoreDetails(qint64 appId)
{
    if (m_storeDetailsCache.contains(appId) || m_storeDetailsInFlight.contains(appId))
        return;

    // Check the on-disk cache before touching the network -- this is what
    // makes startGenreSync()'s whole-library scan cheap after the first
    // launch. Small local JSON reads, done synchronously right here on the
    // calling thread; fast enough not to be worth a background hop.
    const QVariantMap diskDetails = readStoreDetailsFromDisk(appId);
    if (!diskDetails.isEmpty()) {
        m_storeDetailsCache.insert(appId, diskDetails);
        emit storeDetailsReady(appId);

        if (m_warmupAppIds.contains(appId)) {
            const QString trailerUrl = diskDetails.value("trailerUrl").toString();
            if (!trailerUrl.isEmpty())
                requestTrailerCache(appId, trailerUrl);
        }
        return;
    }

    m_storeDetailsInFlight.insert(appId);

    QtConcurrent::run(QThreadPool::globalInstance(), [this, appId]() {
        StoreDetailsClient client;
        QString error;
        const QVariantMap details = client.fetchDetails(appId, error);

        // Marshal back onto this object's own thread (the UI thread) before
        // touching the cache or emitting -- we're running on a thread pool
        // thread here.
        QMetaObject::invokeMethod(this, [this, appId, details]() {
            m_storeDetailsInFlight.remove(appId);
            m_storeDetailsCache.insert(appId, details);
            writeStoreDetailsToDisk(appId, details);
            emit storeDetailsReady(appId);

            // If this fetch was for one of the randomly-chosen startup
            // warmup games, piggyback the trailer cache request onto this
            // same completion path rather than wiring up a separate signal.
            if (m_warmupAppIds.contains(appId)) {
                const QString trailerUrl = details.value("trailerUrl").toString();
                if (!trailerUrl.isEmpty())
                    requestTrailerCache(appId, trailerUrl);
            }
        }, Qt::QueuedConnection);
    });
}

QVariantMap GameLibraryModel::protonDbRatingFor(qint64 appId) const
{
    return m_protonDbCache.value(appId);
}

void GameLibraryModel::requestProtonDbRating(qint64 appId)
{
    if (m_protonDbCache.contains(appId) || m_protonDbInFlight.contains(appId))
        return;

    m_protonDbInFlight.insert(appId);

    QtConcurrent::run(QThreadPool::globalInstance(), [this, appId]() {
        ProtonDbClient client;
        QString error;
        const QVariantMap rating = client.fetchRating(appId, error);

        QMetaObject::invokeMethod(this, [this, appId, rating]() {
            m_protonDbInFlight.remove(appId);
            m_protonDbCache.insert(appId, rating);
            emit protonDbRatingReady(appId);
        }, Qt::QueuedConnection);
    });
}

QString GameLibraryModel::cachedTrailerUrlFor(qint64 appId) const
{
    return m_trailerCache.cachedTrailerUrl(appId);
}

void GameLibraryModel::requestTrailerCache(qint64 appId, const QString &remoteUrl)
{
    m_trailerCache.requestCache(appId, remoteUrl);
}

bool GameLibraryModel::launchGameViaSteamCli(qint64 appId) const
{
    const QString steamBinary = QStandardPaths::findExecutable("steam");
    if (steamBinary.isEmpty())
        return false;

    // startDetached so the launched process survives independently of us
    // (and isn't reaped/killed if Project Dex exits or this call returns).
    return QProcess::startDetached(steamBinary, {"-applaunch", QString::number(appId)});
}

void GameLibraryModel::onGameReady(const GameEntry &entry)
{
    const int idx = indexForAppId(entry.appId);
    if (idx >= 0) {
        m_games[idx] = entry;
        const QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx);
    } else {
        beginInsertRows(QModelIndex(), m_games.size(), m_games.size());
        m_games.append(entry);
        endInsertRows();
        emit countChanged();
    }

    recomputeHeroGames();
}

void GameLibraryModel::onInstallStateChanged()
{
    // Simplest correct approach for now: a full re-sync keeps installed
    // flags accurate. Box art is cache-skipped for anything already on
    // disk, so this doesn't re-download art -- fast even though it's a
    // full re-sync.
    refresh();
}

void GameLibraryModel::onSyncFinished()
{
    m_syncing = false;
    emit syncingChanged();
    recomputeHeroGames();
    warmRandomTrailers();
    startGenreSync();
    recomputeCategoryRows(); // so "Recently Played" shows up immediately
}

int GameLibraryModel::indexForAppId(qint64 appId) const
{
    for (int i = 0; i < m_games.size(); ++i) {
        if (m_games.at(i).appId == appId)
            return i;
    }
    return -1;
}

void GameLibraryModel::recomputeHeroGames()
{
    constexpr int kHeroCount = 5;

    QList<GameEntry> candidates = m_games;

    // Most recently played first; games never played via Steam (0) sort by
    // total playtime instead, so freshly-synced libraries with no play
    // history yet still get sensible hero candidates rather than an empty
    // banner.
    std::sort(candidates.begin(), candidates.end(), [](const GameEntry &a, const GameEntry &b) {
        if (a.rtimeLastPlayed != b.rtimeLastPlayed)
            return a.rtimeLastPlayed > b.rtimeLastPlayed;
        return a.playtimeMinutes > b.playtimeMinutes;
    });

    QVariantList hero;
    for (int i = 0; i < candidates.size() && hero.size() < kHeroCount; ++i) {
        const GameEntry &g = candidates.at(i);
        QVariantMap entry;
        entry["appId"] = g.appId;
        entry["name"] = g.name;
        entry["boxArtPath"] = g.boxArtPath;
        entry["installed"] = g.installed;
        hero.append(entry);
    }

    if (hero != m_heroGames) {
        m_heroGames = hero;
        emit heroGamesChanged();
    }
}

void GameLibraryModel::warmRandomTrailers()
{
    // Only once per app run -- onSyncFinished() also fires after later
    // re-syncs (e.g. triggered by install-state changes), and we don't want
    // to keep picking a fresh random 5 and re-downloading every time.
    if (m_trailerWarmupDone || m_games.isEmpty())
        return;
    m_trailerWarmupDone = true;

    QList<int> indices;
    indices.reserve(m_games.size());
    for (int i = 0; i < m_games.size(); ++i)
        indices.append(i);

    std::shuffle(indices.begin(), indices.end(), *QRandomGenerator::global());

    constexpr int kWarmupCount = 5;
    for (int i = 0; i < indices.size() && i < kWarmupCount; ++i) {
        const qint64 appId = m_games.at(indices.at(i)).appId;
        m_warmupAppIds.insert(appId);
        // requestStoreDetails() dedupes against anything already
        // cached/in-flight (e.g. from the hero banner already having
        // fetched this game) -- its completion handler is what actually
        // kicks off the trailer download once we know the trailer URL.
        requestStoreDetails(appId);
    }
}

QString GameLibraryModel::storeDetailsCacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/store_details";
}

QVariantMap GameLibraryModel::readStoreDetailsFromDisk(qint64 appId) const
{
    QFile file(storeDetailsCacheDir() + "/" + QString::number(appId) + ".json");
    if (!file.open(QIODevice::ReadOnly))
        return QVariantMap();

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return QVariantMap();
    return doc.object().toVariantMap();
}

void GameLibraryModel::writeStoreDetailsToDisk(qint64 appId, const QVariantMap &details) const
{
    const QString dir = storeDetailsCacheDir();
    QDir().mkpath(dir);

    QFile file(dir + "/" + QString::number(appId) + ".json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    file.write(QJsonDocument(QJsonObject::fromVariantMap(details)).toJson(QJsonDocument::Compact));
}

void GameLibraryModel::startGenreSync()
{
    // Once per app run -- like warmRandomTrailers(), onSyncFinished() also
    // fires on later re-syncs (install-state changes) and there's no need
    // to re-walk the whole library every time.
    if (m_genreSyncStarted || m_games.isEmpty())
        return;
    m_genreSyncStarted = true;

    // requestStoreDetails() is a no-op for anything already cached/in-flight
    // and hits the on-disk cache before the network, so this is cheap on
    // any run after the first -- and even on the first run, the thread
    // pool naturally bounds how many of these fetch concurrently rather
    // than firing all of them onto the network at once.
    for (const GameEntry &g : m_games)
        requestStoreDetails(g.appId);
}

void GameLibraryModel::recomputeCategoryRows()
{
    constexpr int kMinGamesForRow = 4;
    constexpr int kMaxCategoryRows = 6;
    constexpr int kMaxGamesPerRow = 20;
    constexpr int kRecentlyPlayedCount = 15;

    QVariantList rows;

    auto toRowEntry = [](const GameEntry &g) {
        QVariantMap entry;
        entry["appId"] = g.appId;
        entry["name"] = g.name;
        entry["boxArtPath"] = g.boxArtPath;
        entry["installed"] = g.installed;
        return entry;
    };

    // "Recently Played" -- always first, same recency/playtime ordering as
    // the hero banner, just with more entries. Stops as soon as it hits a
    // game that's never actually been played, rather than padding the row
    // out with untouched library entries.
    {
        QList<GameEntry> candidates = m_games;
        std::sort(candidates.begin(), candidates.end(), [](const GameEntry &a, const GameEntry &b) {
            if (a.rtimeLastPlayed != b.rtimeLastPlayed)
                return a.rtimeLastPlayed > b.rtimeLastPlayed;
            return a.playtimeMinutes > b.playtimeMinutes;
        });

        QVariantList games;
        for (int i = 0; i < candidates.size() && games.size() < kRecentlyPlayedCount; ++i) {
            const GameEntry &g = candidates.at(i);
            if (g.rtimeLastPlayed == 0 && g.playtimeMinutes == 0)
                break;
            games.append(toRowEntry(g));
        }

        if (!games.isEmpty()) {
            QVariantMap row;
            row["title"] = "Recently Played";
            row["games"] = games;
            rows.append(row);
        }
    }

    // Bucket games by genre/category tag using whatever store details have
    // been fetched so far -- rows fill in progressively as startGenreSync()
    // works through the library in the background.
    QHash<QString, QList<int>> tagToIndices;
    for (int i = 0; i < m_games.size(); ++i) {
        const QVariantMap details = m_storeDetailsCache.value(m_games.at(i).appId);
        if (details.value("available").toBool() != true)
            continue;

        QStringList tags = details.value("genres").toStringList();
        const QStringList cats = details.value("categories").toStringList();
        if (cats.contains("Co-op") || cats.contains("Online Co-op") || cats.contains("Local Co-op"))
            tags.append("Co-op");
        if (cats.contains("PvP") || cats.contains("Online PvP") || cats.contains("Cross-Platform Multiplayer"))
            tags.append("Multiplayer");

        for (const QString &tag : std::as_const(tags))
            tagToIndices[tag].append(i);
    }

    QStringList candidateTags;
    for (auto it = tagToIndices.constBegin(); it != tagToIndices.constEnd(); ++it) {
        if (it.value().size() >= kMinGamesForRow)
            candidateTags.append(it.key());
    }

    // Randomize which categories show and in what order -- a different mix
    // each app launch, same "fun and varied" idea as the trailer warmup.
    // Sorted by a stable per-run hash rather than freshly shuffled, so the
    // row order doesn't visibly jump around as more rows qualify while
    // startGenreSync() is still working through the library.
    std::sort(candidateTags.begin(), candidateTags.end(), [this](const QString &a, const QString &b) {
        return qHash(a, m_categorySeed) < qHash(b, m_categorySeed);
    });

    for (int t = 0; t < candidateTags.size() && rows.size() < 1 + kMaxCategoryRows; ++t) {
        QList<int> indices = tagToIndices.value(candidateTags.at(t));
        std::sort(indices.begin(), indices.end(), [this](int a, int b) {
            return qHash(m_games.at(a).appId, m_categorySeed) < qHash(m_games.at(b).appId, m_categorySeed);
        });

        QVariantList games;
        for (int i = 0; i < indices.size() && games.size() < kMaxGamesPerRow; ++i)
            games.append(toRowEntry(m_games.at(indices.at(i))));

        QVariantMap row;
        row["title"] = candidateTags.at(t);
        row["games"] = games;
        rows.append(row);
    }

    if (rows != m_categoryRows) {
        m_categoryRows = rows;
        emit categoryRowsChanged();
    }
}
