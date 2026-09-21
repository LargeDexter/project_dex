#pragma once

#include <QObject>
#include "game_entry.h"

class SteamClient;
class LibraryScanner;
class ArtworkClient;

// Lives on a background QThread. All the blocking work (Steam API call,
// filesystem scanning, per-game image downloads) happens here so the UI
// thread never stalls. Talks back to GameLibraryModel purely via signals.
class LibrarySyncWorker : public QObject
{
    Q_OBJECT

public:
    explicit LibrarySyncWorker(QObject *parent = nullptr);
    ~LibrarySyncWorker() override;

public slots:
    // Fetches owned games, cross-references install state, fetches box art
    // for every game up front, and emits gameReady() as each one completes.
    // Also starts (once) watching for live install-state changes.
    void startSync();

signals:
    void gameReady(const GameEntry &entry);
    void installStateChanged();
    void syncFinished();
    void syncFailed(const QString &error);

private:
    SteamClient *m_steamClient = nullptr;
    LibraryScanner *m_libraryScanner = nullptr;
    ArtworkClient *m_artworkClient = nullptr;
    bool m_watchingStarted = false;
};
