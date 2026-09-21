#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QFileSystemWatcher>

struct InstalledApp {
    qint64 appId = 0;
    QString name;
    QString installDir;
    qint64 sizeOnDiskBytes = 0;
    QString libraryPath;
};

class LibraryScanner : public QObject
{
    Q_OBJECT

public:
    explicit LibraryScanner(QObject *parent = nullptr);

    // Locates Steam's install root and every library configured in
    // libraryfolders.vdf. Returns false if no Steam installation was found
    // at any of the common Linux install locations.
    bool discoverLibraries();

    QStringList libraryPaths() const { return m_libraryPaths; }

    // Scans every known library's steamapps folder for appmanifest_*.acf
    // files and returns what's actually installed.
    QList<InstalledApp> scanInstalledApps() const;

    // Watches each library's steamapps folder (inotify under the hood via
    // QFileSystemWatcher) and emits installChanged() when a manifest file
    // is added, removed, or modified -- e.g. an install completing.
    void startWatching();

signals:
    void installChanged();

private:
    QStringList m_libraryPaths;
    QFileSystemWatcher m_watcher;
};
